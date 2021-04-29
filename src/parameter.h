/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       parameter.c/h
  * @brief      
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
#pragma once
#include <stddef.h>

// sys/sysinfo.h 查看核心数信息
#include <sys/sysinfo.h>

namespace copnet
{
	namespace parameter
	{
		//协程栈大小
		const static size_t coroutineStackSize = 32 * 1024;

		//获取活跃的epoll_event的数组的初始长度
		static constexpr int epollerListFirstSize = 16;

		//epoll_wait的阻塞时长
		static constexpr int epollTimeOutMs = 100; //10,000

		//监听队列的长度
		constexpr static unsigned backLog = 4096;

		//内存池没有空闲内存块时申请memPoolMallocObjCnt个对象大小的内存块
		static constexpr size_t memPoolMallocObjCnt = 40;

		#define DEFINE_THREAD_NUMS 16
		static int threadNums = DEFINE_THREAD_NUMS?DEFINE_THREAD_NUMS:(::get_nprocs_conf());
	}
	
}

/*

8线程下的性能：

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

性能对比，优化前：
CPU占用异常，且性能：
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