# 存在的问题：
1) 不能正确回收线程
2) 协程封装问题
3) 初始化accept会导致cpu占用率过高，需优化
4) 发送与delete问题

# net_lib
net lib by cpp

```
= context.cc
= coroutine.cc
= epoller.cc
= mstime.cc
= mutex.cc
= copnet_api.cc
= processor.cc
= processor_selector.cc
= scheduler.cc
= socket.cc
= timer.cc
= mempool.h
= objpool.h
= parameter.h
= spinlock_guard.h
= spinlock.h
= utils.h
```

### copnet

| source                | function       | addition                       |
| :-------------------- | :------------- | :----------------------------- |
| context.cc            | 上下文切换实现 | 封装了ucontext的上下文切换操作 |
| coroutine.cc          | 协程运行       |                                |
| epoller.cc            | 封装epoll      | LT水平触发                     |
| mstime.cc             | 时间           |                                |
| mutex.cc              | 读写锁         | 通过原子变量和队列实现读写锁   |
| copnet_api.cc          | api            |                                |
| processor.cc          | 线程           | 存放协程对象并管理其生命周期   |  |
| processor_selector.cc | 协程分配策略   |                                |
| scheduler.cc          | 协程分配       | 将协程分配到线程               |
| socket.cc             | socket         | 封装socket、accept、listen等   |
| timer.cc              | 定时器         | 超时与延时（小根堆实现）       |


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
copnet::Socket listener;
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
    copnet::Socket* conn = new copnet::Socket(listener.accept()); // 接收连接
    conn->setTcpNoDelay(true); // 设置新的socket的tcp协议不使用Nagle算法
    copnet::co_go( // 协程 执行与客户端的通信
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
                std::string ok = "HTTP/1.0 200 OK\r\nServer: copnet/0.1.0\r\n
					Content-Type: text/html\r\n\r\n";
                conn->send(ok.c_str(), ok.size());
                conn->send((void*)&(buf[0]), readNum);
            }
            copnet::co_sleep(100);//需要等一下，否则还没发送完毕就关闭了
            delete conn; // 删除连接
        }
        );
}
```
### copnet api

| function	| describe	| addition	|
| :-	| :-	| :-	|
| co_go()	| 将协程运行到线程上	| 协程栈大小默认为2k，默认通过调度器选择运行在哪个线程	|
| co_sleep()	| 当前协程等待t毫秒后再继续执行	| 	|
| sche_join()	| 等待调度器的结束	| 	|
```
copnet::co_go( []{} , parameter::coroutineStackSize, i ) 指定协程运行在i线程上，协程栈大小为coroutineStackSize

co_go()调用
copnet::Scheduler::getScheduler()->createNewCo(func, stackSize);
copnet::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);

co_sleep()调用
copnet::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);

sche_join()调用
copnet::Scheduler::getScheduler()->join();
copnet::co_sleep(50);

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
```
MemPool类，
MemBlockNode* _freeListHead; // 空闲链表
MemBlockNode* _mallocListHead; // malloc的大内存块链表
size_t _mallocTimes; // 实际malloc的次数
size_t objSize_; // 每个内存块大小

MemPool的构造析构函数就是初始化参数和释放所有内存。
void* MemPool<objSize>::AllocAMemBlock(); // 为内存块申请内存，返回空闲内存链表的头指针
void MemPool<objSize>::FreeAMemBlock(void* block)； // 释放内存，将内存放到空闲链表
```
### ProcessorSelector

```
int curPro_; // 事件管理器数组的下标
int strategy_; // 分发策略
std::vector<Processor*>& processors_; // 事件管理器数组

setStrategy(); // //设置分发任务的策略
Processor* next(); // 事件管理器选择器，决定下一个事件应该放入哪个事件管理器(Processor)中

```

### Context 上下文
```
同协程库中Context的使用方法。
```

### Coroutine 协程
```
同协程库中Coroutine的使用方法。
此外封装了事件管理器(Processor) Processor* pMyProcessor_;
和获取事件管理器指针操作 Processor* getMyProcessor(){return pMyProcessor_;}
```

### epoller 封装epoll
```
int epollFd_; // epoll文件描述符
std::vector<struct epoll_event> activeEpollEvents_; // epoll事件数组

init(); // 初始化epoll
modifyEv(Coroutine* pCo, int fd, int interesEv); //修改Epoller中的事件 epoll_ctl(),EPOLL_CTL_MOD
addEv(Coroutine* pCo, int fd, int interesEv); //向Epoller中添加事件 epoll_ctl(),EPOLL_CTL_ADD
removeEv(Coroutine* pCo, int fd, int interesEv); //从Epoller中移除事件 epoll_ctl(),EPOLL_CTL_DEL
getActEvServ(int timeOutMs, std::vector<Coroutine*>& activeEvServs); // 获取被激活的事件服务,返回errno epoll_wait()
```

### mstime
```
通过struct timeval tv; ::gettimeofday(&tv, 0);获取时间
toLocalTime(); // 计算得到tm结构的时间（年月日等）
timeIntervalFromNow(); // 获取经过的时间
重载时间的比较运算符 <,>,<=,>=
```

### mutex
```
void rlock(); // 读锁
void runlock(); // 解读锁
void wlock(); // 写锁
void wunlock(); // 解写锁
void freeLock(); // 释放锁

int state_; // 锁状态 MU_FREE,MU_READING,MU_WRITING
std::atomic_int readingNum_; // 要读取的任务数
Spinlock lock_; // atomic变量实现的自旋锁
std::queue<Coroutine*> waitingCo_; // 储存等待读写锁的上下文
```

读写锁实现原理：
通过state_描述读写锁的状态（空闲、读、写），原子变量readingNum_保存持有读锁的线程数，waitingCo_是存放等待锁的协程队列，自旋锁lock_保证对前面数据的互斥访问。

获取锁时，
读锁可以在空闲、读情况下获取锁，获取失败会进入协程队列，等待解锁后恢复运行；
写锁可以在空闲情况下获取锁，获取失败会进入协程队列，等待解锁后恢复运行；
释放锁实现：将状态设置为空闲，并将协程从队列弹出运行。

### timer定时器

timerfd是Linux为用户程序提供的一个定时器接口。这个接口基于文件描述符，通过文件描述符的可读事件进行超时通知，所以能够被用于select/poll的应用场景。
```
int timeFd_; // 定时器描述符
char dummyBuf_[TIMER_DUMMYBUF_SIZE]; // read timefd上的数据
TimerHeap timerCoHeap_; // 定时器协程集合（小顶堆）

bool init(Epoller*); // 创建timefd并添加到epoll上
void getExpiredCoroutines(std::vector<Coroutine*>& expiredCoroutines); // 获取所有已经超时的需要执行的函数
void runAt(Time time, Coroutine* pCo); // 在time时刻需要恢复协程co
void runAfter(Time time, Coroutine* pCo); // 经过time毫秒恢复协程co
void wakeUp(); // 唤醒定时器
bool resetTimeOfTimefd(Time time); // 给timefd重新设置时间，time是绝对时间
inline bool isTimeFdUseful() { return timeFd_ < 0 ? false : true; }; // 文件描述符是否有效
```

### parameter 参数
```
const static size_t coroutineStackSize = 32 * 1024; // 协程栈大小
static constexpr int epollerListFirstSize = 16; // 获取活跃的epoll_event的数组的初始长度
static constexpr int epollTimeOutMs = 10000; // epoll_wait的阻塞时长
constexpr static unsigned backLog = 4096; // 监听队列的长度
static constexpr size_t memPoolMallocObjCnt = 40; // 内存池没有空闲内存块时申请memPoolMallocObjCnt个对象大小的内存块
```
### spinlock_guard Spinlock 自旋锁
```
Spinlock 对原子变量封装lock unlock操作，作为二元信号量。
spinlock_guard 封装Spinlock，构造和析构分别对应加锁、解锁操作
```
### utils
```
声明DISALLOW_COPY_MOVE_AND_ASSIGN
```
