/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       spinlock_guard.c/h
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
#include "spinlock.h"
#include "utils.h"

namespace netco {

	//配合std::atomic_int定义的二元信号量使用，为1表示资源可以使用，为0表示资源不可使用
	class SpinlockGuard
	{
	public:
		SpinlockGuard(Spinlock& l)
			: lock_(l)
		{
			lock_.lock();
		}

		~SpinlockGuard()
		{
			lock_.unlock();
		}

		DISALLOW_COPY_MOVE_AND_ASSIGN(SpinlockGuard);

	private:
		Spinlock& lock_;

	};

}
