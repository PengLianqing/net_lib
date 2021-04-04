/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       client.c/h
  * @brief      使用方法：export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
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
#include <iostream>
#include <sys/sysinfo.h>

#include "../include/processor.h"
#include "../include/netco_api.h"
#include "../include/socket.h"
#include "../include/mutex.h"

#include <atomic>

using namespace netco;

/**
  * @brief          client_test
  * 使用四线程+协程调度测试服务器端性能
  */
std::atomic_int32_t times(0);
void client_test(){
	for(int i=0;i<2;++i){
		netco::co_go(
		[i]{
			for (int j = 0; j < 10000; ++j){
				netco::co_go(
					[ j ]
					{
						char buf[1024];
						// std::cout<<j<<std::endl; // 协程的第j个连接
						netco::co_sleep(0); // 协程切换
						netco::Socket s;
						s.connect("127.0.0.1", 7103);
						for(int k=50;k>0;--k){
							s.send("ping", 4);
							s.read(buf, 1024);
							// std::cout << buf << std::endl;
							netco::co_sleep(0);//需要等一下，否则还没发送完毕就关闭了
						}
						times.fetch_add(1);
						// std::cout<<times.load()<<std::endl;
					} 
				);
			}
		}
		,parameter::coroutineStackSize, i);
	}

	while( times.load()<20000 )
	{
		::sleep(0);
	}
	std::cout  << "all done. times:" << times.load() << std::endl;
}


int main()
{
	client_test();
	// std::this_thread::sleep_for( std::chrono::milliseconds(10000) );
	// netco::sche_join();
	std::cout << "end" << std::endl;
	return 0;
}
