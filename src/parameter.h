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