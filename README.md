# net_lib
net lib by cpp

### netco

| source					| function		| addition						|
| :------------------------ | :------------ | :---------------------------- |
| context.cc				| 上下文切换实现	| 封装了ucontext的上下文切换操作	|
| coroutine.cc				| 协程运行	| 	 |
| epoller.cc				| 封装epoll	| LT水平触发	|
| mstime.cc					| 时间	| 	|
| mutex.cc					| 读写锁	| 通过原子变量和队列实现读写锁 	|
| netco_api.cc				| api	|	|
| processor.cc				| 线程	| 存放协程对象并管理其生命周期| 	|
| processor_selector.cc		| 协程分配策略	| 	|
| scheduler.cc				| 协程分配	| 将协程分配到线程	|
| socket.cc					| socket	| 封装socket、accept、listen等	|
| timer.cc					| 定时器	| 超时与延时（小根堆实现）	|

根据计算机核心数开启对应数量的线程，每一个线程对应一个Processor实例，协程Coroutine实例运行在Processor的主循环中，Procesor使用epoll和定时器timer进行任务调度。

Scheduler是一个全局单例，在某个线程中调用co_go()运行一个新协程后，实际会调用该实例的方法，选择协程最少的（或其他策略）Processor接管新的协程。

### socket.cc/h
socket类封装ip地址、端口、引用计数；

| function	| describe	| addition	|
| :-	| :-	| :-	|
| getSocketOpt()/getSocketOptString()	| 获取套接字的选项	| 	|
| bind/listen/accept/connect/read/send	| socket封装	| 添加到epoll对应事件后才执行系统调用	|
| shutdownWrite()	| 关闭socket的写端	|
| setTcpNoDelay()	| 设置TCP是否使用Nagle算法	|通过减少需要传输的数据包，来优化网络
| setReuseAddr()/setReusePort()	| 设置socket的ip地址和端口重用	|
| setKeepAlive()	| 设置socket是否使用心跳检测	|
| setNonBolckSocket()/setBlockSocket()	| 设置socket阻塞	|

使用：
服务器端：
```
netco::Socket listener;
if (listener.isUseful()) // 判断socket是否有效
{
    listener.setTcpNoDelay(true); // 设置socket的tcp协议不使用Nagle算法
    listener.setReuseAddr(true); // 地址、端口复用
    listener.setReusePort(true);
    if (listener.bind(8099) < 0) // 绑定地址结构
    {
        return;
    }
    listener.listen(); // 监听
}
while (1){
    netco::Socket* conn = new netco::Socket(listener.accept()); // 接收连接
    conn->setTcpNoDelay(true); // 设置新的socket的tcp协议不使用Nagle算法
    netco::co_go( // 协程 执行与客户端的通信
        [conn]
        {
            std::vector<char> buf;
            buf.resize(2048);
            while (1)
            {
                // read
                auto readNum = conn->read((void*)&(buf[0]), buf.size());
                if(readNum < 0){
                    break;
                }
                // send
                std::string ok = "HTTP/1.0 200 OK\r\nServer: netco/0.1.0\r\n
					Content-Type: text/html\r\n\r\n";
                conn->send(ok.c_str(), ok.size());
                conn->send((void*)&(buf[0]), readNum);
            }
            netco::co_sleep(100);//需要等一下，否则还没发送完毕就关闭了
            delete conn; // 删除连接
        }
        );
}
```
### netco api

| function	| describe	| addition	|
| :-	| :-	| :-	|
| co_go()	| 将协程运行到线程上	| 协程栈大小默认为2k，默认通过调度器选择运行在哪个线程	|
| co_sleep()	| 当前协程等待t毫秒后再继续执行	| 	|
| sche_join()	| 等待调度器的结束	| 	|
```
netco::co_go( []{} , parameter::coroutineStackSize, i ) 指定协程运行在i线程上，协程栈大小为coroutineStackSize

co_go()调用
netco::Scheduler::getScheduler()->createNewCo(func, stackSize);
netco::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);

co_sleep()调用
netco::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);

sche_join()调用
netco::Scheduler::getScheduler()->join();
netco::co_sleep(50);

```

### Scheduler
Scheduler类封装了互斥元、线程数组、时间管理器选择器
```
static Scheduler* pScher_;	//日志管理器实例
static std::mutex scherMtx_;
std::vector<Processor*> processors_;
ProcessorSelector proSelector_;

createNewCo(); // 在idx号线程创建新协程
getProcessor(); // 返回数组中对应线程对象
join(); // 回收所有线程
getScheduler(); // 返回Scheduler指针
startScheduler(); // 根据线程数量向线程数组中创建Processor对象

```

### Processor
```
int tid_; // 该Processor的线程号
int status_; // 该Processor的运行状态
std::thread* pLoop_; // 线程对象
std::queue<Coroutine*> newCoroutines_[2]; //新任务双缓存队列。一个用于存放正在运行的协程，另一个用于添加任务。
volatile int runningNewQue_; // newCoroutines_双缓冲选择
Spinlock newQueLock_; // newCoroutines_的自旋锁
std::vector<Coroutine*> actCoroutines_; // EventEpoller发现的活跃事件所放的列表
std::set<Coroutine*> coSet_; // set存放新来的协程地址，用于方便查找是否有这个协程
std::vector<Coroutine*> timerExpiredCo_; // 定时器任务列表
std::vector<Coroutine*> removedCo_; // 被移除的协程列表，要移除某一个事件会先放在该列表中，一次循环结束才会真正delete
Epoller epoller_; // epoll对象
Timer timer_; // 定时器对象，epoll实现
ObjPool<Coroutine> coPool_; // 存放上下文对象，ObjPool实现
Spinlock coPoolLock_; // coPool_的自旋锁
Coroutine* pCurCoroutine_; // 存放当前要执行的协程的上下文
Context mainCtx_; // 线程loop的上下文

Processor（）; // 构造函数，初始化线程的上下文
~Processor(); // 析构函数，停止并回收线程
resume(Coroutine* pCo)； // 恢复运行指定的协程
yield(); // 暂停运行当前协程。执行切换到线程的loop，继续进行协程调度
wait(); // 当前协程等待time毫秒。执行切换到线程的loop，time后恢复运行协程
goCo(Coroutine* pCo); // 将上下文添加到新任务缓冲队列
goCoBatch(std::vector<Coroutine*>& cos)； // 将上下文数组添加到新任务缓冲队列    
loop(); // 线程任务：初始化epoller_(epoll对象)、timer_(定时器对象)、pLoop_(线程对象)。循环：获取epoll的活跃事件，处理超时的协程，执行新来的协程,执行被唤醒的协程，然后清除清除已经执行完毕的协程。
waitEvent(); // 等待fd上的ev事件返回
stop(); // 停止线程(终止了线程循环while条件)
join(); // 回收线程
wakeUpEpoller(); // 唤醒epoller
goNewCo(); // 运行一个新协程，添加到coPool_
killCurCo(); // 删除协程，添加到removedCo_被移除的协程列表

```

### ObjPool

通过内存池，创建对象、申请内存或删除对象、释放内存。

通过integral_constant类实现。
integral_constant类是所有traits类的基类，分别提供了以下功能：
value_type 表示值的类型
value表示值
type 表示自己, 因此可以用::type::value来获取值
true_type和false_type两个特化类用来表示bool值类型的traits，很多traits类都需要继承它们。

```
MemPool<sizeof(T)> _memPool; // 内存池
inline T* ObjPool<T>::new_obj(Args... args) // 创建一个新的对象
inline T* ObjPool<T>::new_aux(std::true_type, Args... args) // 申请内存 true_type
inline T* ObjPool<T>::new_aux(std::false_type, Args... args) // 申请内存 false_type
inline void ObjPool<T>::delete_obj(void* obj) // 删除对象
inline void ObjPool<T>::delete_aux(std::true_type, void* obj) // 释放内存 true_type
inline void ObjPool<T>::delete_aux(std::false_type, void* obj) // 释放内存 false_type
```

### mempool
MemBlockNode结构体，里面封装了共用体union，有指向下一个节点的指针和字符。
```
struct MemBlockNode
{
	union
	{
		MemBlockNode* next;
		char data;
	};
};
```
共用体的所有成员占用同一段内存，修改一个成员会影响其余所有成员。
共用体占用的内存等于最长的成员占用的内存。共用体使用了内存覆盖技术，同一时刻只能保存一个成员的值，如果对新的成员赋值，就会把原来成员的值覆盖掉。

MemPool类，
MemBlockNode* _freeListHead; // 空闲链表
MemBlockNode* _mallocListHead; // malloc的大内存块链表
size_t _mallocTimes; // 实际malloc的次数
size_t objSize_; // 每个内存块大小

MemPool的构造析构函数就是初始化参数和释放所有内存。
void* MemPool<objSize>::AllocAMemBlock(); // 为内存块申请内存，返回空闲内存链表的头指针
void MemPool<objSize>::FreeAMemBlock(void* block)； // 释放内存，将内存放到空闲链表