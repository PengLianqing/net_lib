/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       server.c/h
  * @brief      使用：export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
				使用：sudo ./webbench -c 13000 -t 10  http://211.67.19.174:7103/
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

			copnet::co_go(
			[]
			{
				while(true){
					// std::cout << std::this_thread::get_id() << " running timefd " << std::endl;
					printCoNums();
					std::cout << "connections:" << times.load() << std::endl;
					copnet::co_sleep(1*1000); // 200ms
				}
			}
			,parameter::coroutineStackSize, 0);
		
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

			while(true)
			{
				copnet::Socket* conn = new copnet::Socket(listener.accept());
				conn->setTcpNoDelay(true);
				times.fetch_add(1);
				copnet::co_go(
					[conn]
					{
						std::string hello("HTTP/1.0 200 OK\r\nServer: copnet/0.1.0\r\nContent-Length: 72\r\nContent-Type: \
							text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
						// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
						char buf[1024];
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

void multi_acceptor_server_test()
{
	auto tCnt = parameter::threadNums; // 获取核心数
	
	copnet::co_go(
		[]
		{
			while(true){
				// std::cout << std::this_thread::get_id() << " running timefd " << std::endl;
				printCoNums();
				std::cout << "connections:" << times.load() << std::endl;
				copnet::co_sleep(1*1000); // 200ms
			}
		}
	,parameter::coroutineStackSize, 0);

	for (int i = 0; i < tCnt; ++i) // 每个核心建立一个线程执行accept
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

				while(true)
				{
					copnet::Socket* conn = new copnet::Socket(listener.accept());
					conn->setTcpNoDelay(true);
					times.fetch_add(1);
					copnet::co_go(
						[conn]
						{
							// std::cout << std::this_thread::get_id() << " running server " << std::endl;

							std::string hello("HTTP/1.0 200 OK\r\nServer: copnet/0.1.0\r\nContent-Length: 72\r\nContent-Type: \
								text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							char buf[1024];
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

/*

* 性能测试：

	多线程epoll接收：
	peng@ubuntu:~/code/net_lib/webbench/webbench-1.5$ ./webbench -c 5000 -t 10  http://211.67.19.174:7103/
	Webbench - Simple Web Benchmark 1.5
	Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.
	Benchmarking: GET http://211.67.19.174:7103/
	5000 clients, running 10 sec.
	Speed=5351238 pages/min, 13623638 bytes/sec.
	Requests: 891873 susceed, 0 failed.

	单线程epoll接收：
	peng@ubuntu:~/code/net_lib/webbench/webbench-1.5$ ./webbench -c 5000 -t 10  http://211.67.19.174:7103/
	Webbench - Simple Web Benchmark 1.5
	Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.
	Benchmarking: GET http://211.67.19.174:7103/
	5000 clients, running 10 sec.
	Speed=4546818 pages/min, 11590319 bytes/sec.
	Requests: 757803 susceed, 0 failed.

* 性能对比：

	优化前：CPU占用异常，且性能：
	单线程epoll接收（16线程处理）：
	Speed=2719266 pages/min, 5954065 bytes/sec.
	Requests: 453211 susceed, 0 failed.
	多（16）线程epoll接收（16线程处理）：
	Speed=4921068 pages/min, 12443880 bytes/sec.
	Requests: 820178 susceed, 0 failed.

	优化后：CPU占用正常，且性能：
	单线程epoll接收（8线程处理）：
	Speed=4546818 pages/min, 11590319 bytes/sec.
	Requests: 757803 susceed, 0 failed.
	多（8）线程epoll接收（8线程处理）：
	Speed=5351238 pages/min, 13623638 bytes/sec.
	Requests: 891873 susceed, 0 failed.

	单线程epoll接收（16线程处理）：
	Speed=5060610 pages/min, 12885754 bytes/sec.
	Requests: 843435 susceed, 0 failed.
	多（16）线程epoll接收（16线程处理）：
	Speed=5385486 pages/min, 13704867 bytes/sec.
	Requests: 897581 susceed, 0 failed.

*/

int main()
{
	single_acceptor_server_test();

	// multi_acceptor_server_test();

	copnet::sche_join(); // 回收线程

	std::cout << "end" << std::endl;

	return 0;
}