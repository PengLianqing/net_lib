/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       timer.c/h
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
#include "timer.h"
#include "coroutine.h"
#include "epoller.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

using namespace copnet;

Timer::Timer()
	: timeFd_(-1)
{}

Timer::~Timer() 
{
	if (isTimeFdUseful())
	{
		::close(timeFd_);
	}
}

#include <iostream>
bool Timer::init(Epoller* pEpoller)
{
	timeFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	// std::cout << "timerfd " << timeFd_ << std::endl ;
	if (isTimeFdUseful())
	{
		return pEpoller->addEv(nullptr, timeFd_, EPOLLIN | EPOLLPRI | EPOLLRDHUP);
	}
	return false;
}

void Timer::getExpiredCoroutines(std::vector<Coroutine*>& expiredCoroutines)
{
	/*
		测试timefd的触发次数
	*/
	static long times = 0;

	Time nowTime = Time::now();
	// 将超时的事件弹出
	while (!timerCoHeap_.empty() && timerCoHeap_.top().first <= nowTime)
	{
		expiredCoroutines.push_back(timerCoHeap_.top().second);
		timerCoHeap_.pop();
	}
	// 读完timeFd_来到的数据（epoll水平触发）
	if ( 1 ) // !expiredCoroutines.empty()) // epoll 异常原因
	{
		ssize_t cnt = TIMER_DUMMYBUF_SIZE;
		while (cnt >= TIMER_DUMMYBUF_SIZE)
		{
			cnt = ::read(timeFd_, dummyBuf_, TIMER_DUMMYBUF_SIZE);
			// if( ++times%10==0 ){ // 1000000
			// 	std::cout << " time fd " << times++ << " " << cnt <<std::endl;
			// }
		}
	}
	// 更新定时器
	if (!timerCoHeap_.empty())
	{
		Time time = timerCoHeap_.top().first;
		resetTimeOfTimefd(time);
	}
}

void Timer::runAt(Time time, Coroutine* pCo)
{
	timerCoHeap_.push(std::move(std::pair<Time, Coroutine*>(time, pCo)));
	if (timerCoHeap_.top().first == time)
	{//如果新加入的任务是最紧急的任务则需要更改timefd所设置的时间
		resetTimeOfTimefd(time);
	}
}

//给timefd重新设置时间，time是绝对时间
bool Timer::resetTimeOfTimefd(Time time)
{
	struct itimerspec newValue;
	struct itimerspec oldValue;
	memset(&newValue, 0, sizeof newValue);
	memset(&oldValue, 0, sizeof oldValue);
	newValue.it_value = time.timeIntervalFromNow();
	int ret = ::timerfd_settime(timeFd_, 0, &newValue, &oldValue);
	return ret < 0 ? false : true;
}

void Timer::runAfter(Time time, Coroutine* pCo)
{
	Time runTime(Time::now().getTimeVal() + time.getTimeVal());
	runAt(runTime, pCo);
}

void Timer::wakeUp()
{
	resetTimeOfTimefd(Time::now());
}