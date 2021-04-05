/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       mutex.c/h
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
#include "coroutine.h"
#include "spinlock.h"

#include <atomic>
#include <queue>

namespace copnet{

    enum muStatus
	{
		MU_FREE = 0,
		MU_READING,
		MU_WRITING
	};

    //读写锁
    class RWMutex{
    public:
        RWMutex()
            : state_(MU_FREE), readingNum_(0)
        {};
        ~RWMutex(){};

        DISALLOW_COPY_MOVE_AND_ASSIGN(RWMutex);

        //读锁
        void rlock();
        //解读锁
        void runlock();

        //写锁
        void wlock();
        //解写锁
        void wunlock();

    private:
        void freeLock();

        int state_;

        std::atomic_int readingNum_;

        Spinlock lock_;

        std::queue<Coroutine*> waitingCo_;

    };

}