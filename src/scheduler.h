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
#pragma once
#include <vector>
#include <functional>

#include "processor.h"
#include "processor_selector.h"

namespace copnet
{

	class Scheduler
	{
	protected:
		Scheduler();
		~Scheduler();

	public:

		DISALLOW_COPY_MOVE_AND_ASSIGN(Scheduler);

		static Scheduler* getScheduler();

		//在idx号线程创建新协程
		void createNewCo(std::function<void()>&& func, size_t stackSize);
		void createNewCo(std::function<void()>& func, size_t stackSize);

		Processor* getProcessor(int); // 返回数组中对应线程对象

		int getProCnt();

		void join(); // 回收所有线程

		void printCoNums();
		
	private:
		//初始化Scheduler，threadCnt为开启几个线程
		bool startScheduler(int threadCnt);

		//日志管理器实例
		static Scheduler* pScher_;

		//用于保护的锁，为了服务器执行效率，原则上不允许长久占有此锁
		static std::mutex scherMtx_;

		std::vector<Processor*> processors_;

		ProcessorSelector proSelector_;
	};

}