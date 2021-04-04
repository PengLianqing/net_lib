/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       netco_api.c/h
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
#include "../include/netco_api.h"

void netco::co_go(std::function<void()>&& func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		netco::Scheduler::getScheduler()->createNewCo(std::move(func), stackSize);
	}
	else
	{
		tid %= netco::Scheduler::getScheduler()->getProCnt();
		netco::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(std::move(func), stackSize);
	}
}

void netco::co_go(std::function<void()>& func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		netco::Scheduler::getScheduler()->createNewCo(func, stackSize);
	}
	else
	{
		tid %= netco::Scheduler::getScheduler()->getProCnt();
		netco::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);
	}
}

void netco::co_sleep(Time time)
{
	netco::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

void netco::sche_join()
{
	netco::Scheduler::getScheduler()->join();
}