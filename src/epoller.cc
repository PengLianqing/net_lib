/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       epoller.c/h
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
#include "epoller.h"
#include "coroutine.h"
#include "parameter.h"

#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

/*
	epoll 测试
*/
#include <iostream>
#include <thread>

using namespace copnet;

Epoller::Epoller()
	: epollFd_(-1), activeEpollEvents_(parameter::epollerListFirstSize)
{ }

Epoller::~Epoller()
{
	if (isEpollFdUseful())
	{
		::close(epollFd_);
	}
};

/*

140201624086336epollFd_ 3
timerfd 4
3 add fd 
140201624086336epollFd_ 5
timerfd 6
5 add fd 
140201624086336epollFd_ 7
timerfd 8
7 add fd 
140201624086336epollFd_ 9
timerfd 10
9 add fd 
3 add fd 
5 add fd 
7 add fd 
connections:0
9 add fd 
connections:0
connections:0
connections:0
140201607296768 epoll 7 1000031 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 2000030 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 3000028 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201615689472 epoll 5 4000029 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 5000030 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201615689472 epoll 5 6000026 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 7000028 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 8000027 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
140201607296768 epoll 7 9000030 1
16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

*/
bool Epoller::init()
{
	// epoll_create1() 使用FD_CLOEXEC实现close-on-exec，关闭子进程无用文件描述符
	// 即设置该epoll描述符的close-on-exec(FD_CLOEXEC)标志。
	// https://blog.csdn.net/ChrisNiu1984/article/details/7050663
	epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
	// epollFd_ = ::epoll_create(activeEpollEvents_.size()+1);
	std::cout << std::this_thread::get_id() << "epollFd_ " << epollFd_ << std::endl;
	return isEpollFdUseful();
}

//修改Epoller中的事件
bool Epoller::modifyEv(Coroutine* pCo, int fd, int interesEv)
{
	if (!isEpollFdUseful())
	{
		return false;
	}
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.events = interesEv;
	event.data.ptr = pCo;
	if (::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
	{
		return false;
	}
	return true;
}

#include <iostream>
//向Epoller中添加事件
bool Epoller::addEv(Coroutine* pCo, int fd, int interesEv)
{
	// std::cout << epollFd_ << " add fd " << std::endl;
	if (!isEpollFdUseful())
	{
		return false;
	}
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.events = interesEv;
	event.data.ptr = pCo;
	if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
	{
		return false;
	}
	return true;
}

static int times;
//从Epoller中移除事件
bool Epoller::removeEv(Coroutine* pCo, int fd, int interesEv)
{
	// std::cout<<"remove"<<std::endl;
	if (!isEpollFdUseful())
	{
		return false;
	}
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.events = interesEv;
	event.data.ptr = pCo;
	if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
	{
		return false;
	}
	return true;
}

int Epoller::getActEvServ(int timeOutMs, std::vector<Coroutine*>& activeEvServs)
{
	if (!isEpollFdUseful())
	{
		return -1;
	}
	// int actEvNum = ::epoll_wait(epollFd_, &*activeEpollEvents_.begin(), static_cast<int>(activeEpollEvents_.size()), timeOutMs);
	
	int actEvNum = ::epoll_wait(epollFd_, & activeEpollEvents_[0], static_cast<int>(activeEpollEvents_.size()), timeOutMs);
	
	// ::usleep(10*1000);
	// std::cout<<times++<<","<<actEvNum<<std::endl;
	
	// if( ++times%1000000==0 ){
	//  	std::cout<<times++<<" " <<actEvNum<<std::endl;
	// }

	int savedErrno = errno;
	if (actEvNum > 0)
	{
		/*******
		此处为了观察添加了延时，会影响并发性能。
		********/
		// ::usleep(10); 
		if( ++times%1000000==0 ){ // 1000000
			std::cout  << std::this_thread::get_id() << " epoll " << epollFd_ << " " << times++ << " " << actEvNum << std::endl; 

			// std::cout << activeEpollEvents_.size() << "," ;
			// for(auto elem:activeEpollEvents_){
			// 	std::cout << elem.data.fd << ",";
			// }
			// std::cout << std::endl;
		}
	
		if (actEvNum > static_cast<int>(activeEpollEvents_.size()))
		{
			return savedErrno;
		}
		for (int i = 0; i < actEvNum; ++i)
		{
			//设置事件类型，放进活跃事件列表中
			Coroutine* pCo = static_cast<Coroutine*>(activeEpollEvents_[i].data.ptr);
			activeEvServs.push_back(pCo);
		}
		if (actEvNum == static_cast<int>(activeEpollEvents_.size()))
		{
			//若从epoll中获取事件的数组满了，说明这个数组的大小可能不够，扩展一倍
			activeEpollEvents_.resize(activeEpollEvents_.size() * 2);
		}
	}
	return savedErrno;
}