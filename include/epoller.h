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
#pragma once
#include "utils.h"

#include <vector>

struct epoll_event;

namespace netco
{
	class Coroutine;

	class Epoller
	{
	public:
		Epoller();
		~Epoller();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Epoller);

		//要使用EventEpoller必须调用该函数初始化，失败则返回false
		bool init();

		//修改Epoller中的事件
		bool modifyEv(Coroutine* pCo, int fd, int interesEv);

		//向Epoller中添加事件
		bool addEv(Coroutine* pCo, int fd, int interesEv);

		//从Epoller中移除事件
		bool removeEv(Coroutine* pCo, int fd, int interesEv);

		//获取被激活的事件服务,返回errno
		int getActEvServ(int timeOutMs, std::vector<Coroutine*>& activeEvServs);

	private:

		inline bool isEpollFdUseful() { return epollFd_ < 0 ? false : true; };

		int epollFd_; // epoll文件描述符

		std::vector<struct epoll_event> activeEpollEvents_; // epoll事件数组

	};

}
