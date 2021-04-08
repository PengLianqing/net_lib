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

bool Epoller::init()
{
	// epoll_create1() 使用FD_CLOEXEC实现close-on-exec，关闭子进程无用文件描述符
	// 即设置该epoll描述符的close-on-exec(FD_CLOEXEC)标志。
	// https://blog.csdn.net/ChrisNiu1984/article/details/7050663
	epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
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

//向Epoller中添加事件
bool Epoller::addEv(Coroutine* pCo, int fd, int interesEv)
{
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

/*
	epoll 测试
*/
#include <iostream>
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
	int actEvNum = ::epoll_wait(epollFd_, &*activeEpollEvents_.begin(), static_cast<int>(activeEpollEvents_.size()), timeOutMs);
	
	// ::usleep(10*1000);
	// std::cout<<times++<<","<<actEvNum<<std::endl;
	// if(++times%10000==0){
	// 	std::cout<<times++<<std::endl;
	// }
	int savedErrno = errno;
	if (actEvNum > 0)
	{
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