/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       copnet_api.c/h
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
#include "copnet_api.h"

/**
  * @brief          co_go 运行协程.
  * 不指定tid(-1)时,默认通过ProcessorSelector选择线程运行协程,指定tid时运行到指定的线程;
  * stackSize默认为(32*1024).
  */
void copnet::co_go(std::function<void()>&& func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		copnet::Scheduler::getScheduler()->createNewCo(std::move(func), stackSize);
	}
	else
	{
		tid %= copnet::Scheduler::getScheduler()->getProCnt();
		copnet::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(std::move(func), stackSize);
	}
}

void copnet::co_go(std::function<void()>& func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		copnet::Scheduler::getScheduler()->createNewCo(func, stackSize);
	}
	else
	{
		tid %= copnet::Scheduler::getScheduler()->getProCnt();
		copnet::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);
	}
}

/**
  * @brief          printCoNums 获取各个线程运行的协程数.
  */
void copnet::printCoNums()
{
	copnet::Scheduler::getScheduler()->printCoNums();
}

/**
  * @brief          co_sleep 协程休眠指定时间(ms).
  * 通过timefd和epoll实现定时.
  */
void copnet::co_sleep(Time time)
{
	copnet::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

/**
  * @brief          sche_join 回收线程资源.
  */
void copnet::sche_join()
{
	copnet::Scheduler::getScheduler()->join();
}