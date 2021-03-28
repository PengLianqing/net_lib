//@Author Liu Yukang 
#include "../include/socket.h"
#include "../include/scheduler.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>  // snprintf
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

using namespace netco;

Socket::~Socket()
{
	--(*_pRef);
	if (!(*_pRef) && isUseful())
	{
		::close(_sockfd);
		delete _pRef;
	}
}

bool Socket::getSocketOpt(struct tcp_info* tcpi) const
{
	socklen_t len = sizeof(*tcpi);
	memset(tcpi, 0, sizeof(*tcpi));
	return ::getsockopt(_sockfd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getSocketOptString(char* buf, int len) const
{
	struct tcp_info tcpi;
	bool ok = getSocketOpt(&tcpi);
	if (ok)
	{
		snprintf(buf, len, "unrecovered=%u "
			"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
			"lost=%u retrans=%u rtt=%u rttvar=%u "
			"sshthresh=%u cwnd=%u total_retrans=%u",
			tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
			tcpi.tcpi_rto,          // Retransmit timeout in usec
			tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
			tcpi.tcpi_snd_mss,
			tcpi.tcpi_rcv_mss,
			tcpi.tcpi_lost,         // Lost packets
			tcpi.tcpi_retrans,      // Retransmitted packets out
			tcpi.tcpi_rtt,          // Smoothed round trip time in usec
			tcpi.tcpi_rttvar,       // Medium deviation
			tcpi.tcpi_snd_ssthresh,
			tcpi.tcpi_snd_cwnd,
			tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
	}
	return ok;
}

std::string Socket::getSocketOptString() const
{
	char buf[1024];
	buf[0] = '\0';
	getSocketOptString(buf, sizeof buf);
	return std::string(buf);
}


int Socket::bind(int port)
{
	_port = port;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(struct sockaddr_in));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = ::bind(_sockfd, (struct sockaddr*) & serv, sizeof(serv));
	return ret;
}

int Socket::listen()
{
	int ret = ::listen(_sockfd, parameter::backLog);
	return ret;
}

Socket Socket::accept_raw()
{
	int connfd = -1;
	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	connfd = ::accept(_sockfd, (struct sockaddr*) & client, &len);
	if (connfd < 0)
	{
		return Socket(connfd);
	}

	//accept成功保存用户ip
	struct sockaddr_in* sock = (struct sockaddr_in*) & client;
	int port = ntohs(sock->sin_port);  //linux上打印方式
	struct in_addr in = sock->sin_addr;
	char ip[INET_ADDRSTRLEN];   //INET_ADDRSTRLEN这个宏系统默认定义 16
	//成功的话此时IP地址保存在str字符串中。
	inet_ntop(AF_INET, &in, ip, sizeof(ip));

	return Socket(connfd, std::string(ip), port);
}

/*
EPOLLIN ：表示监听读事件（包括对端SOCKET正常关闭）；
EPOLLOUT：表示监听写事件；
EPOLLPRI：表示监听紧急数据可读事件（这里应该表示有带外数据到来）；
EPOLLERR：表示监听错误事件；
EPOLLHUP：表示监听挂断事件；
EPOLLRDHUP：读关闭事件
EPOLLHUP：读写关闭事件

EPOLLET：   将epoll设为边沿触发(Edge Triggered)模式，该参数缺省状态为水平触发LT。
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socketfd的话，需重新把这个socketfd加入到EPOLL队列里。
*/
Socket Socket::accept(){
	auto ret(accept_raw());
	if(ret.isUseful()){
		return ret;
	}
	netco::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(_sockfd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
	auto con(accept_raw());
	if(con.isUseful()){
		return con;
	}
	return accept();
}

//从socket中读数据
ssize_t Socket::read(void* buf, size_t count)
{
	auto ret = ::read(_sockfd, buf, count);
	if (ret >= 0){
		return ret;
	}
	if(ret == -1 && errno == EINTR){
		return read(buf, count);
	}
	netco::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(_sockfd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
	return ::read(_sockfd, buf, count);
}

void Socket::connect(const char* ip, int port){
	struct sockaddr_in addr = {0};
	addr.sin_family= AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	_ip = std::string(ip);
	_port = port;
	auto ret = ::connect(_sockfd, (struct sockaddr*)&addr, sizeof(sockaddr_in));
	if(ret == 0){
		return;
	}
	if(ret == -1 && errno == EINTR){
		return connect(ip, port);
	}
	netco::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(_sockfd, EPOLLOUT);
	return connect(ip, port);
}

//往socket中写数据
ssize_t Socket::send(const void* buf, size_t count)
{
	//忽略SIGPIPE信号
	size_t sendIdx = ::send(_sockfd, buf, count, MSG_NOSIGNAL);
	if (sendIdx >= count){
		return count;
	}
	netco::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(_sockfd, EPOLLOUT);
	return send((char *)buf + sendIdx, count - sendIdx);
}

int Socket::shutdownWrite()
{
	int ret = ::shutdown(_sockfd, SHUT_WR);
	return ret;
}

int Socket::setTcpNoDelay(bool on)
{
	int optval = on ? 1 : 0;
	// 是否使用Nagle算法
	// TCP/IP协议中针对TCP默认开启了Nagle算法。Nagle算法通过减少需要传输的数据包，来优化网络。在内核实现中，数据包的发送和接受会先做缓存，分别对应于写缓存和读缓存。
	int ret = ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY,
		&optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

int Socket::setReuseAddr(bool on)
{
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR,
		&optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

int Socket::setReusePort(bool on)
{
	int ret = -1;
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT,
		&optval, static_cast<socklen_t>(sizeof optval));
#endif
	return ret;
}

int Socket::setKeepAlive(bool on)
{
	// 设置socket是否使用心跳检测
	/*
	对于面向连接的TCP socket,在实际应用中通常都要检测对端是否处于连接中,连接端口分两种情况:
		1、连接正常关闭,调用close() shutdown()连接正常关闭,send与recv立马返回错误,select返回SOCK_ERR;
		2、连接的对端异常关闭
	对于第二种情况，支持使用SO_KEEPALIVE来做心跳检测，

	keepalive原理:TCP内嵌有心跳包,以服务端为例,当server检测到超过一定时间(/proc/sys/net/ipv4/tcp_keepalive_time 7200 即2小时)没有数据传输,那么会向client端发送一个keepalive packet,此时client端有三种反应:
	1、client端连接正常,返回一个ACK.server端收到ACK后重置计时器,在2小时后在发送探测.如果2小时内连接上有数据传输,那么在该时间的基础上向后推延2小时发送探测包;
	2、客户端异常关闭,或网络断开。client无响应,server收不到ACK,在一定时间(/proc/sys/net/ipv4/tcp_keepalive_intvl 75 即75秒)后重发keepalive packet, 并且重发一定次数(/proc/sys/net/ipv4/tcp_keepalive_probes 9 即9次);
	3、客户端曾经崩溃,但已经重启.server收到的探测响应是一个复位,server端终止连接。
	参数修改：
	临时方法:向三个文件中直接写入参数,系统重启需要重新设置;
	临时方法:sysctl -w net.ipv4.tcp_keepalive_intvl=20
	全局设置:可更改/etc/sysctl.conf,加上:
		net.ipv4.tcp_keepalive_intvl = 20
		net.ipv4.tcp_keepalive_probes = 3
		net.ipv4.tcp_keepalive_time = 60
	*/
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE,
		&optval, static_cast<socklen_t>(sizeof optval));
	return ret;
}

//设置socket为非阻塞的
int Socket::setNonBolckSocket()
{
	auto flags = fcntl(_sockfd, F_GETFL, 0);
	int ret = fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式
	return ret;
}

//设置socket为阻塞的
int Socket::setBlockSocket()
{
	auto flags = fcntl(_sockfd, F_GETFL, 0);
	int ret = fcntl(_sockfd, F_SETFL, flags & ~O_NONBLOCK);    //设置成阻塞模式；
	return ret;
}