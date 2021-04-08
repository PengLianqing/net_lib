/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       processor.c/h
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
#include "processor.h"
#include "parameter.h"
#include "spinlock_guard.h"

#include <sys/epoll.h>
#include <unistd.h>

using namespace copnet;

__thread int threadIdx = -1;

Processor::Processor(int tid)
	: tid_(tid), status_(PRO_STOPPED), pLoop_(nullptr), runningNewQue_(0), pCurCoroutine_(nullptr), mainCtx_(0)
{
	mainCtx_.makeCurContext();
}

Processor::~Processor()
{
	if (PRO_RUNNING == status_)
	{
		stop();
	}
	if (PRO_STOPPING == status_)
	{
		join();
	}
	if (nullptr != pLoop_)
	{
		delete pLoop_;
	}
	for (auto co : coSet_)
	{
		delete co;
	}
}

/**
  * @brief          恢复运行指定的协程
  * 传入参数：协程的上下文
  */
void Processor::resume(Coroutine* pCo)
{
	if (nullptr == pCo)
	{
		return;
	}

	if (coSet_.find(pCo) == coSet_.end()) // 查看对应协程是否在coSet_中
	{
		return;
	}
	
	pCurCoroutine_ = pCo;
	pCo->resume();
}

/**
  * @brief    暂停运行当前协程    
  *  执行：切换到线程的loop，继续进行协程调度
  */
void Processor::yield()
{
	pCurCoroutine_->yield();
	mainCtx_.swapToMe(pCurCoroutine_->getCtx()); // 将上下文保存到pCurCoroutine_中 ， 然后切换到当前上下文mainCtx_
}

/**
  * @brief    当前协程等待time毫秒   
  * 执行：切换到线程的loop，time后恢复运行协程   
  */
void Processor::wait(Time time)
{
	pCurCoroutine_->yield();
	timer_.runAfter(time,pCurCoroutine_); // 经过time毫秒恢复协程co
	mainCtx_.swapToMe(pCurCoroutine_->getCtx()); // 将上下文保存到pCurCoroutine_中 ， 然后切换到当前上下文mainCtx_
}

/**
  * @brief    将上下文添加到新任务缓冲队列      
  */
void Processor::goCo(Coroutine* pCo)
{
	{
		SpinlockGuard lock(newQueLock_);
		newCoroutines_[!runningNewQue_].push(pCo);  // 将上下文添加到新任务缓冲队列(双缓冲)
	}
	wakeUpEpoller();
}

/**
  * @brief    将上下文数组添加到新任务缓冲队列      
  */
void Processor::goCoBatch(std::vector<Coroutine*>& cos){
	{
		SpinlockGuard lock(newQueLock_);
		for(auto pCo : cos){
			newCoroutines_[!runningNewQue_].push(pCo);
		}
	}
	wakeUpEpoller();
}

#include <iostream>
bool Processor::loop()
{
	//初始化Epoller
	if (!epoller_.init())
	{
		return false;
	}

	//初始化Timer
	if (!timer_.init(&epoller_))
	{
		return false;
	}

	//初始化loop
	pLoop_ = new std::thread(
		[this]
		{
			threadIdx = tid_;
			status_ = PRO_RUNNING;
			while (PRO_RUNNING == status_)
			{
				//清空所有列表
				if (actCoroutines_.size())
				{
					actCoroutines_.clear();
				}
				if (timerExpiredCo_.size())
				{
					timerExpiredCo_.clear();
				}
				//获取活跃事件
				epoller_.getActEvServ(parameter::epollTimeOutMs, actCoroutines_);
				//处理超时协程 
				timer_.getExpiredCoroutines(timerExpiredCo_);
				size_t timerCoCnt = timerExpiredCo_.size();
				for (size_t i = 0; i < timerCoCnt; ++i)
				{
					resume(timerExpiredCo_[i]);
				}

				//执行新来的协程
				Coroutine* pNewCo = nullptr;
				int runningQue = runningNewQue_;
				
				while (!newCoroutines_[runningQue].empty())
				{
					{
						pNewCo = newCoroutines_[runningQue].front();
						newCoroutines_[runningQue].pop();
						coSet_.insert(pNewCo);
					}
					resume(pNewCo);
				}

				{
					SpinlockGuard lock(newQueLock_);
					runningNewQue_ = !runningQue;
				}

				//执行被唤醒的协程
				size_t actCoCnt = actCoroutines_.size();
				for (size_t i = 0; i < actCoCnt; ++i)
				{
					resume(actCoroutines_[i]);
				}

				//清除已经执行完毕的协程
				for (auto deadCo : removedCo_)
				{
					coSet_.erase(deadCo);
					//delete deadCo;
					{
						SpinlockGuard lock(coPoolLock_);
						coPool_.delete_obj(deadCo);
					}
				}
				removedCo_.clear();
				
			}
			status_ = PRO_STOPPED;
		}
		);
	return true;
}

//等待fd上的ev事件返回
void Processor::waitEvent(int fd, int ev){
	epoller_.addEv(pCurCoroutine_, fd, ev);
	yield();
	epoller_.removeEv(pCurCoroutine_, fd, ev);
}

void Processor::stop()
{
	status_ = PRO_STOPPING;
}

void Processor::join()
{
	pLoop_->join();
}

void Processor::wakeUpEpoller()
{
	timer_.wakeUp();
}

void Processor::goNewCo(std::function<void()>&& coFunc, size_t stackSize)
{
	//Coroutine* pCo = new Coroutine(this, stackSize, std::move(coFunc));
	Coroutine* pCo = nullptr;

	{
		SpinlockGuard lock(coPoolLock_);
		pCo = coPool_.new_obj(this, stackSize, std::move(coFunc));
	}

	goCo(pCo);
}

void Processor::goNewCo(std::function<void()>& coFunc, size_t stackSize)
{
	//Coroutine* pCo = new Coroutine(this, stackSize, coFunc);
	Coroutine* pCo = nullptr;

	{
		SpinlockGuard lock(coPoolLock_);
		pCo = coPool_.new_obj(this, stackSize, coFunc);
	}
	
	goCo(pCo);
}

void Processor::killCurCo()
{
	removedCo_.push_back(pCurCoroutine_);
}
