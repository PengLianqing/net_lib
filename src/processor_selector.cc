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
#include "processor_selector.h"
#include "processor.h"
#include <iostream>
using namespace copnet;

Processor* ProcessorSelector::next()
{
	int n = static_cast<int>(processors_.size());
	if (!n)
	{
		return nullptr;
	}
	// 线程0不做安排
	int minCoProIdx = 1;
	size_t minCoCnt = processors_.front()->getCoCnt();
	switch (strategy_)
	{
	case MIN_EVENT_FIRST:
		for (int i = 1; i < n ; ++i)
		{
			size_t coCnt = processors_[i]->getCoCnt();
			if (coCnt < minCoCnt)
			{
				minCoCnt = coCnt;
				minCoProIdx = i;
			}
		}
		curPro_ = minCoProIdx;
		break;

	case ROUND_ROBIN:
	default:
		++curPro_;
		if (curPro_ >= n)
		{
			// 线程0不做安排
			curPro_ = 1;
		}
		break;
	}
	// std::cout << " go to thread " << curPro_ << std::endl;
	return processors_[curPro_];
};

void ProcessorSelector::printCoNums(){

	int n = static_cast<int>(processors_.size());
	for( int i=0;i<n;++i ){
		size_t coCnt = processors_[i]->getCoCnt();
		std::cout << coCnt << ",";
	} std::cout << std::endl;
}