/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       server.c/h
  * @brief      使用：export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     Jan-1-2021      Peng            1. 完成
  *
  @verbatim
  ==============================================================================
  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2021 Peng****************************
  */
#include <iostream>
#include <sys/sysinfo.h>

#include "../src/processor.h"
#include "../src/copnet_api.h"
#include "../src/socket.h"
#include "../src/mutex.h"

using namespace copnet;

std::atomic_int32_t times(0);
// 只有一个acceptor的服务
void single_acceptor_server_test()
{
	copnet::co_go(
		[]{
			copnet::Socket listener;
			if (listener.isUseful()) // 判断socket是否有效
			{
				listener.setTcpNoDelay(true); // 设置socket的tcp协议不使用Nagle算法
				listener.setReuseAddr(true); // 地址、端口复用
				listener.setReusePort(true);
				if (listener.bind(7103) < 0) // 绑定地址结构
				{
					return;
				}
				listener.listen(); // 监听
			}
			copnet::co_go(
					[]
					{
						while(true){
							std::cout << "connections:" << times.load() << std::endl;
							copnet::co_sleep(200);
						}
					}
			);
			while(true)
			{
				copnet::Socket* conn = new copnet::Socket(listener.accept());
				conn->setTcpNoDelay(true);
				times.fetch_add(1);
				copnet::co_go(
					[conn]
					{
						std::string hello("HTTP/1.0 200 OK\r\nServer: copnet/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
						// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
						char buf[1024];
						/*
						int i = 0;
						while (conn->read((void*)buf, 1024) > 0)
						{
							++i;
							// copnet::co_sleep( rand()%1000 ); // 稍后回复
							conn->send(hello.c_str(), hello.size());
							copnet::co_sleep(0);
							// std::cout << "messages:" << i << std::endl;
						}
						*/
						if (conn->read((void*)buf, 1024) > 0)
						{
							conn->send(hello.c_str(), hello.size());
							copnet::co_sleep(50); // 等待，防止提前关闭socket
						}
						delete conn;
						times.fetch_sub(1);
					}
				);
			}
		}
	);
}

//每条线程一个acceptor的服务
/*
multi_acceptor_server_test函数执行逻辑：
程序开始，获取核心数，根据核心数创建协程，返回“#########return ”
其实执行到#########return 的时候已经交给sheduler托管了

无连接的时候会炸cpu，因为此时频繁进行协程切换（accept没有反应，切换上下文，继续没有反应，继续切换上下文） 需要优化
有accept后，cpu不会爆炸(最起码accept有问题)
但是感觉协程并没有成功回收。->找到协程在哪里显示数量显示一下，时间函数。

首先测试协程返回
*/
/*
问题原因：epoll_wait超高频次调用;
bool Processor::loop()执行了八次，
有八个神奇的东西被添加到了epoll，暂时还没找到哪的错误.
在epoll中设置了remove和触发次数的统计。
processor 的pLoop_ = new std::thread(也是在一直执行，有关联。
::usleep(10*1000);
std::cout<<times++<<","<<actEvNum<<std::endl;
connections:0
243,1
244,1
245,1
246,1
247,1
有一个epoll监听对象一直在触发。
*/
/*
下一个优化：sheduler中始终初始化的是8（核心数）的线程，优化到用户定义阶段。(实际可能只使用一个，考虑情况)
*/
void multi_acceptor_server_test()
{
	auto tCnt = ::get_nprocs_conf(); // 获取核心数
	//for (int i = 0; i < tCnt; ++i) // 每个核心建立一个线程执行accept
	for (int i = 0; i < 1; ++i) // 使用四线程测试
	{
		copnet::co_go(
			[]
			{
				copnet::Socket listener;
				if (listener.isUseful())
				{
					listener.setTcpNoDelay(true);
					listener.setReuseAddr(true);
					listener.setReusePort(true);
					if (listener.bind(7103) < 0)
					{
						return;
					}
					listener.listen();
				}
				copnet::co_go(
					[]
					{
						while(true){
							std::cout << "connections:" << times.load() << std::endl;
							copnet::co_sleep(200); // 200ms
						}
					}
				);
				while(true)
				{
					copnet::Socket* conn = new copnet::Socket(listener.accept());
					conn->setTcpNoDelay(true);
					times.fetch_add(1);
					copnet::co_go(
						[conn]
						{
							std::string hello("HTTP/1.0 200 OK\r\nServer: copnet/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							char buf[1024];
							/*
							int i = 0;
							while (conn->read((void*)buf, 1024) > 0)
							{
								++i;
								// copnet::co_sleep( rand()%1000 ); // 稍后回复
								conn->send(hello.c_str(), hello.size());
								copnet::co_sleep(0);
								// std::cout << "messages:" << i << std::endl;
							}
							*/
							if (conn->read((void*)buf, 1024) > 0)
							{
								conn->send(hello.c_str(), hello.size());
								copnet::co_sleep(50); // 等待，防止提前关闭socket
							}
							delete conn;
							times.fetch_sub(1);
						}
					);
				}
			}
			,parameter::coroutineStackSize, i);
	}
}

void test(){
	copnet::co_go(
		[]{
			for(int j=0;j<20;++j){
				std::cout << "new" << std::endl;
				copnet::co_go(
					[]
					{
						while(true){
							static int i = 0;
							// std::cout << "connections:" << times.load() << std::endl;
							std::cout << "times" << ++i << std::endl;
							copnet::co_sleep(200); // 200ms
						}
					}
				);
			}
		}
	);
}
/*
单线程与多线程性能差距：
测试场景：20000个连接，每个连接进行50次"ping"操作
peng@ubuntu:~/code/net_lib/client$ time ./client 
all done. times:20000

real    0m8.604s
user    0m2.016s
sys     0m16.169s
peng@ubuntu:~/code/net_lib/client$ time ./client 
all done. times:20000

real    0m17.610s
user    0m6.888s
sys     0m27.837s
*/
int main()
{
	// single_acceptor_server_test();

	multi_acceptor_server_test();

	// test();

	copnet::sche_join();

	std::cout << "end" << std::endl;
	return 0;
}