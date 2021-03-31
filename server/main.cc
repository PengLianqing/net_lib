#include <iostream>
#include <sys/sysinfo.h>

#include "../include/processor.h"
#include "../include/netco_api.h"
#include "../include/socket.h"
#include "../include/mutex.h"

using namespace netco;

// 运行
// export LD_LIBRARY_PATH=/home/peng/Share/myprj/openSourcePrj/netco/src/
// export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
// ./netco_test -lnetco

// netco http response with one acceptor test 
// 只有一个acceptor的服务
void single_acceptor_server_test()
{
	netco::co_go(
		[]{
			netco::Socket listener;
			if (listener.isUseful()) // 判断socket是否有效
			{
				listener.setTcpNoDelay(true); // 设置socket的tcp协议不使用Nagle算法
				listener.setReuseAddr(true); // 地址、端口复用
				listener.setReusePort(true);
				if (listener.bind(8099) < 0) // 绑定地址结构
				{
					return;
				}
				listener.listen(); // 监听
			}
			while (1){
				netco::Socket* conn = new netco::Socket(listener.accept()); // 接收连接
				conn->setTcpNoDelay(true); // 设置新的socket的tcp协议不使用Nagle算法
				netco::co_go( // 协程 执行通信
					[conn]
					{
						std::vector<char> buf;
						buf.resize(2048);
						while (1)
						{
							auto readNum = conn->read((void*)&(buf[0]), buf.size());
							std::string ok = "HTTP/1.0 200 OK\r\nServer: netco/0.1.0\r\nContent-Type: text/html\r\n\r\n";
							if(readNum < 0){
								break;
							}
							conn->send(ok.c_str(), ok.size());
							conn->send((void*)&(buf[0]), readNum);
							if(readNum < (int)buf.size()){
								break;
							}
						}
						netco::co_sleep(100);//需要等一下，否则还没发送完毕就关闭了
						delete conn;
					}
					);
			}
		}
	);
}

//netco http response with multi acceptor test 
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
std::atomic_int32_t times(0);
void multi_acceptor_server_test()
{
	auto tCnt = ::get_nprocs_conf(); // 获取核心数
	for (int i = 0; i < tCnt; ++i) // 每个核心建立一个线程执行accept
	{
		netco::co_go(
			[]
			{
				netco::Socket listener;
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
				int j = 0;
				// while ( j-->0 )
				while(true)
				{
					netco::Socket* conn = new netco::Socket(listener.accept());
					conn->setTcpNoDelay(true);
					++j;
					netco::co_go(
						[conn,j]
						{
							std::cout << j << "start" << std::endl;
							// 所谓的此处重复执行是指多线程重复了
							std::string hello("HTTP/1.0 200 OK\r\nServer: netco/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							char buf[1024];
							if (conn->read((void*)buf, 1024) > 0)
							{
								netco::co_sleep( rand()%1000 ); // 稍后回复
								conn->send(hello.c_str(), hello.size());
								netco::co_sleep(50);//需要等一下，否则还没发送完毕就关闭了
							}

							times.fetch_add(1);

							std::cout << j << "done" << times.load() << std::endl;
							delete conn;
						}
					);
				}
			}
			,parameter::coroutineStackSize, i);
	}
	std::cout << "#########return " << std::endl;
}

int main()
{
	// single_acceptor_server_test();
	multi_acceptor_server_test();
	netco::sche_join();
	std::cout << "end" << std::endl;
	return 0;
}