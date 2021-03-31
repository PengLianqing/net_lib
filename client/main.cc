//@author Liu Yukang
#include <iostream>
#include <sys/sysinfo.h>

#include "../include/processor.h"
#include "../include/netco_api.h"
#include "../include/socket.h"
#include "../include/mutex.h"

#include <atomic>

using namespace netco;

// export LD_LIBRARY_PATH=/home/peng/Share/myprj/openSourcePrj/netco/src/
// export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
// ./netco_test -lnetco

// 客户端
// 使用四线程+协程调度测试服务器性能
std::atomic_int32_t times(0);
void client_test(){
	for(int i=0;i<4;++i){
		netco::co_go(
		[i]{
			for (int j = 0; j < 10000; ++j){
				netco::co_go(
					[ j ]
					{
						char buf[1024];
						// std::cout<<j<<std::endl; // 协程的第j个连接
						netco::co_sleep(1); // 协程切换
						netco::Socket s;
						s.connect("127.0.0.1", 7103);
						s.send("ping", 4);
						s.read(buf, 1024);
						
						times.fetch_add(1);
						// std::cout<<times.load()<<std::endl;
					} 
				);
			}
		}
		,parameter::coroutineStackSize, i);
	}

	while( times.load()<40000 )
	{
		::sleep(0);
	}
	std::cout  << "all done. times:" << times.load() << std::endl;
}

/*
问题出在不能回收线程
不存在协程前部分执行出错的情况
*/
int main()
{
	client_test();
	// std::this_thread::sleep_for( std::chrono::milliseconds(10000) );
	// netco::sche_join();
	// std::cout << "end" << std::endl;
	return 0;
}
