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
#pragma once
#include "scheduler.h"
#include "mstime.h"

namespace copnet 
{
	/* copnet api */
	
	void co_go(std::function<void()>& func, size_t stackSize = parameter::coroutineStackSize, int tid = -1);

	void co_go(std::function<void()>&& func, size_t stackSize = parameter::coroutineStackSize, int tid = -1);

	void printCoNums();

	void co_sleep(Time t);

	void sche_join();

}