#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>
#include "Util.h"
#include "base/Logging.h"

using namespace std;

__thread EventLoop *t_loopInThisThread = 0; // 线程局部变量，每个线程独立拥有一个EventLoop指针

// 创建一个eventfd，用于线程之间的事件通知
int EventLoop::createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // 创建一个eventfd，设置为非阻塞和执行时关闭
    if (evtfd < 0)
    {                               // 检查eventfd是否创建成功
        LOG << "Failed in eventfd"; // 记录错误日志
        abort();                    // 终止程序
    }
    return evtfd; // 返回eventfd的文件描述符
}

// EventLoop构造函数
EventLoop::EventLoop()
    : looping_(false),                 // 初始化looping_为false，表示事件循环未开始
      poller_(new Epoll()),            // 创建一个Epoll对象，用于管理事件
      wakeupFd_(createEventfd()),      // 创建一个eventfd，用于唤醒事件循环
      quit_(false),                    // 初始化quit_为false，表示事件循环未退出
      eventHandling_(false),           // 初始化eventHandling_为false，表示未处理事件
      callingPendingFunctors_(false),  // 初始化callingPendingFunctors_为false，表示未调用待处理的回调函数
      threadId_(CurrentThread::tid()), // 获取当前线程ID
      pwakeupChannel_(new Channel(this, wakeupFd_))
{ // 创建一个Channel对象，绑定到wakeupFd_
    if (t_loopInThisThread)
    { // 检查是否已有EventLoop对象在当前线程中
      // LOG << "Another EventLoop " << t_loopInThisThread << " exists in this
      // thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this; // 将当前EventLoop对象指针赋值给线程局部变量
    }
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);                       // 设置wakeupChannel_的事件类型为EPOLLIN和EPOLLET
    pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this)); // 设置读事件处理器为handleRead函数
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this)); // 设置连接处理器为handleConn函数
    poller_->epoll_add(pwakeupChannel_, 0);                              // 将wakeupChannel_添加到epoll中
}

// 处理连接事件
void EventLoop::handleConn()
{
    updatePoller(pwakeupChannel_, 0); // 更新epoll中的wakeupChannel_
}

// EventLoop析构函数
EventLoop::~EventLoop()
{
    close(wakeupFd_);          // 关闭eventfd
    t_loopInThisThread = NULL; // 将线程局部变量置为NULL
}

// 唤醒函数，向eventfd写数据以唤醒阻塞的epoll_wait
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char *)(&one), sizeof one); // 向eventfd写入数据，触发读事件
    if (n != sizeof one)
    {                                                                       // 检查写入数据的大小是否正确
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8"; // 记录错误日志
    }
}

// 处理读事件，从eventfd读取数据
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one); // 从eventfd读取数据，清除读事件
    if (n != sizeof one)
    {                                                                          // 检查读取数据的大小是否正确
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8"; // 记录错误日志
    }
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET); // 重新设置wakeupChannel_的事件类型
}

// 在事件循环线程中运行回调函数
void EventLoop::runInLoop(Functor &&cb)
{
    if (isInLoopThread()) // 如果当前线程是事件循环线程
        cb();             // 直接执行回调函数
    else
        queueInLoop(std::move(cb)); // 否则将回调函数加入待处理队列
}

// 将回调函数加入到待执行队列，并在必要时唤醒事件循环线程
void EventLoop::queueInLoop(Functor &&cb)
{
    {
        MutexLockGuard lock(mutex_);                  // 加锁保护待处理队列
        pendingFunctors_.emplace_back(std::move(cb)); // 将回调函数加入待处理队列
    }

    if (!isInLoopThread() || callingPendingFunctors_) // 如果当前线程不是事件循环线程或正在调用待处理的回调函数
        wakeup();                                     // 唤醒事件循环线程
}

// 事件循环，处理事件、执行回调函数等
void EventLoop::loop()
{
    assert(!looping_);           // 确保事件循环未开始
    assert(isInLoopThread());    // 确保在事件循环线程中调用
    looping_ = true;             // 设置事件循环开始标志
    quit_ = false;               // 设置事件循环未退出标志
    std::vector<SP_Channel> ret; // 创建一个Channel共享指针的向量，用于存储就绪的事件
    while (!quit_)
    {                             // 当未退出事件循环时
        ret.clear();              // 清空向量
        ret = poller_->poll();    // 调用epoll_wait，等待事件就绪
        eventHandling_ = true;    // 设置事件处理标志
        for (auto &it : ret)      // 遍历就绪的事件
            it->handleEvents();   // 处理每个事件
        eventHandling_ = false;   // 清除事件处理标志
        doPendingFunctors();      // 执行待处理的回调函数，就是处理该loop上的新连接，在server.c中将channel和http_data绑定完会添加进这里待处理的队列中，
                                  // 这里会在每次唤醒fd对应的pwakeupChannel_有事件发生时，会激活epoll_wait函数然后执行，执行该函数，架构队列中待加入事件加入红黑树上
        poller_->handleExpired(); // 处理超时事件
    }
    looping_ = false; // 清除事件循环开始标志
}

// 执行所有待处理的回调函数
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;  // 创建一个回调函数的向量
    callingPendingFunctors_ = true; // 设置调用待处理回调函数标志

    {
        MutexLockGuard lock(mutex_);     // 加锁保护待处理队列
        functors.swap(pendingFunctors_); // 将待处理队列中的回调函数移动到functors向量中
    }

    for (size_t i = 0; i < functors.size(); ++i) // 遍历所有回调函数
        functors[i]();                           // 执行每个回调函数
    callingPendingFunctors_ = false;             // 清除调用待处理回调函数标志
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true; // 设置退出标志
    if (!isInLoopThread())
    {             // 如果当前线程不是事件循环线程
        wakeup(); // 唤醒事件循环线程，以便退出循环
    }
}
