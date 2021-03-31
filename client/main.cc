//@author Liu Yukang
#include <iostream>
#include <sys/sysinfo.h>

#include "../include/processor.h"
#include "../include/netco_api.h"
#include "../include/socket.h"
#include "../include/mutex.h"

using namespace netco;

// export LD_LIBRARY_PATH=/home/peng/Share/myprj/openSourcePrj/netco/src/
// export LD_LIBRARY_PATH=/home/peng/code/net_lib/src/
// ./netco_test -lnetco

//作为客户端的测试，可配合上述server测试
void client_test(){
	netco::co_go(
		[]{
			for (int i = 0; i < 1; ++i){
				netco::co_go(
						[ i ]
						{
							char buf[1024];
							int j = 10;
							while(j-->0){
								std::cout<<j<<std::endl; // 协程的第j个连接
								netco::co_sleep(1); // 协程切换
								netco::Socket s;
								s.connect("127.0.0.1", 7103);
								s.send("ping", 4);
								s.read(buf, 1024);
								// std::cout << std::string(buf) << std::endl;
							}
						} 
				// );
				,parameter::coroutineStackSize, i);
			}
		}
		);
}

/*
问题出在不能回收线程
不存在协程前部分执行出错的情况
*/
int main()
{
	client_test();
	// std::this_thread::sleep_for( std::chrono::milliseconds(10000) );
	netco::sche_join();
	std::cout << "end" << std::endl;
	return 0;
}
