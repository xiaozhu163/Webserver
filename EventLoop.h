#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "Util.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"
#include "base/Thread.h"

#include <iostream>
using namespace std;

class EventLoop
{
public:
    typedef std::function<void()> Functor; // 回调函数
    EventLoop();
    ~EventLoop();
    void loop();                                                                 // 开始事件循环 调⽤该函数的线程必须是该 EventLoop 所在线程，也就是 Loop 函数不能跨线程调⽤
    void quit();                                                                 // 退出事件循环
    void runInLoop(Functor &&cb);                                                // 运行回调函数 如果当前线程就是创建此EventLoop的线程 就调⽤callback(关闭连接 EpollDel) 否则就放⼊等待执⾏函数区
    void queueInLoop(Functor &&cb);                                              // 添加回调函数 把此函数放⼊等待执⾏函数区 如果当前是跨线程 或者正在调⽤等待的函数则唤醒
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }    // 判断是否在当前线程
    void assertInLoopThread() { assert(isInLoopThread()); }                      // 断⾔在当前线程
    void shutdown(shared_ptr<Channel> channel) { shutDownWR(channel->getFd()); } // 关闭写端
    void removeFromPoller(shared_ptr<Channel> channel)
    {
        // shutDownWR(channel->getFd());
        // 从epoll内核事件表中删除fd及其绑定的事件
        poller_->epoll_del(channel);
    }
    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        // 在epoll内核事件表更新fd所绑定的事件
        poller_->epoll_mod(channel, timeout);
    }
    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        // 把fd和绑定的事件注册到epoll内核事件表
        poller_->epoll_add(channel, timeout);
    }

private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_

    bool looping_;                // 是否处于事件循环
    bool quit_;                   // 是否退出事件循环
    bool eventHandling_;          // 是否正在处理事件
    bool callingPendingFunctors_; // 是否正在处理等待的函数

    shared_ptr<Epoll> poller_;             // io多路复⽤ 分发器。 poller_ 用于事件循环中，通过 epoll 机制等待和分发 IO 事件。它负责注册、修改和删除文件描述符上的事件，处理事件的多路复用。
    int wakeupFd_;                         // ⽤于异步唤醒 SubLoop 的 Loop 函数中的Poll(epoll_wait因为还没有注册fd会⼀直阻塞)
    mutable MutexLock mutex_;              // 互斥锁 用于保护 pendingFunctors_，确保多线程环境下对 pendingFunctors_ 的安全访问。
    std::vector<Functor> pendingFunctors_; // 正在等待处理的函数 存储那些等待执行的任务。这些任务可以由其他线程提交到 EventLoop 中。
    const pid_t threadId_;                 // 线程id threadId_ 是 EventLoop 所在线程的 ID，用于确保某些操作只能在特定的线程中执行。
    shared_ptr<Channel> pwakeupChannel_;   // ⽤于异步唤醒的 channel 专门用于处理 wakeupFd_ 的事件。当需要唤醒 EventLoop 时，通过 pwakeupChannel_ 触发相应的回调。

    static int createEventfd(); // 创建eventfd 类似管道的 进程间通信⽅式
    void wakeup();              // 异步唤醒SubLoop的epoll_wait(向event_fd中写⼊数据)
    void handleRead();          // eventfd的读回调函数(因为event_fd有可读数据，所以触发可读事件，从event_fd读数据)
    void doPendingFunctors();   // 执⾏正在等待的函数(SubLoop注册EpollAdd连接套接字以及绑定事件的函数)
    void handleConn();          // 处理连接，实现更新事件，eventfd的更新事件回调函数(更新监听事件)
};
