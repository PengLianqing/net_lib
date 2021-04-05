/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       scheduler.c/h
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
#include "scheduler.h"

#include <sys/sysinfo.h>

using namespace copnet;

Scheduler* Scheduler::pScher_ = nullptr;
std::mutex Scheduler::scherMtx_;

Scheduler::Scheduler()
	:proSelector_(processors_)
{ }

Scheduler::~Scheduler()
{
	for (auto pP : processors_)
	{
		pP->stop();
	}
	for (auto pP : processors_)
	{
		pP->join();
		delete pP;
	}
}

bool Scheduler::startScheduler(int threadCnt)
{
	for (int i = 0; i < threadCnt; ++i)
	{
		processors_.emplace_back(new Processor(i));
		processors_[i]->loop();
	}
	return true;
}

Scheduler* Scheduler::getScheduler()
{
	// 如果pScher_已初始化返回指针，否则初始化
	if (nullptr == pScher_) 
	{
		std::lock_guard<std::mutex> lock(scherMtx_);
		if (nullptr == pScher_)
		{
			pScher_ = new Scheduler();
			pScher_->startScheduler(::get_nprocs_conf()); // 初始化pScher_
		}
	}
	return pScher_;
}

void Scheduler::createNewCo(std::function<void()>&& func, size_t stackSize)
{
	proSelector_.next()->goNewCo(std::move(func), stackSize);
}

void Scheduler::createNewCo(std::function<void()>& func, size_t stackSize)
{
	proSelector_.next()->goNewCo(func, stackSize);
}

void Scheduler::join()
{
	for (auto pP : processors_)
	{
		pP->join();
	}
}

Processor* Scheduler::getProcessor(int id)
{
	return processors_[id];
}

int Scheduler::getProCnt()
{
	// static_cast是一个c++运算符，功能是把一个表达式转换指定类型
	return static_cast<int>(processors_.size());
}