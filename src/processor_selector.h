/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       processor_selector.c/h
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
#include <iostream>
namespace copnet
{
	class Processor;

	enum scheduleStrategy
	{
		MIN_EVENT_FIRST = 0 , //最少事件优先
		ROUND_ROBIN			  //轮流分发
	};

	//事件管理器选择器，决定下一个事件应该放入哪个事件管理器中
	class ProcessorSelector
	{
	public:
		ProcessorSelector(std::vector<Processor*>& processors, int strategy = ROUND_ROBIN) :  curPro_(-1) , strategy_(strategy) , processors_(processors) {}
		~ProcessorSelector() {}

		//设置分发任务的策略
		//MIN_EVENT_FIRST则每次挑选EventService最少的EventManager接收新连接
		//ROUND_ROBIN则每次轮流挑选EventManager接收新连接
		inline void setStrategy(int strategy) { strategy_ = strategy; };

		Processor* next();

		void printCoNums();

	private:
		int curPro_; // 事件管理器数组的下标

		int strategy_; // 分发任务的策略

		std::vector<Processor*>& processors_; // Processor对象数组

	};

}