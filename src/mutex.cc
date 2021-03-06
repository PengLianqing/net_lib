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
#include "mutex.h"
#include "scheduler.h"
#include "spinlock_guard.h"

using namespace copnet;

void RWMutex::rlock(){
    // 成功获取到自旋锁，readingNum_+1
    // 否则将正在运行的协程添加到waitingCo_队列，切换到线程的协程调度后重试
    {
        SpinlockGuard l(lock_);
        if(state_ == MU_FREE || state_ == MU_READING){
            readingNum_.fetch_add(1);
            state_ = MU_READING;
            return;
        }

        waitingCo_.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
    
    }

    Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
    rlock();

}

void RWMutex::runlock(){
    // 成功获取到自旋锁，然后readingNum_-1
    // 当readingNum_为1，则freeLock()释放锁
    SpinlockGuard l(lock_);
    auto cur = readingNum_.fetch_add(-1);
    if(cur == 1){
        freeLock();
    }
}

void RWMutex::wlock(){
    // 成功获取到自旋锁，如果锁未被占用，获取锁
    // 否则将正在运行的协程添加到waitingCo_队列，切换到线程的协程调度后重试
    {
        SpinlockGuard l(lock_);
        if(state_ == MU_FREE){
            state_ = MU_WRITING;
            return;
        }
        
        waitingCo_.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
        
    }
    
    Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
    wlock();
}
    
void RWMutex::wunlock(){
    // 成功获取到自旋锁，后freeLock()释放锁
    SpinlockGuard l(lock_);
    freeLock();
}

void RWMutex::freeLock(){
    // 释放锁
    // 将之前等待锁的任务从任务队列弹出，通过协程运行
    state_ = MU_FREE;
    while(!waitingCo_.empty()){
        auto wakeCo = waitingCo_.front();
        waitingCo_.pop();
        wakeCo->getMyProcessor()->goCo(wakeCo);
    }
}