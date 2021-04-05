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

void copnet::co_sleep(Time time)
{
	copnet::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

void copnet::sche_join()
{
	copnet::Scheduler::getScheduler()->join();
}