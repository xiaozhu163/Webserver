#include "EventLoopThread.h"
#include <functional> // 包含函数对象的头文件

EventLoopThread::EventLoopThread()
    : loop_(NULL),                                                               // 初始化事件循环指针为NULL
      exiting_(false),                                                           // 初始化退出标志为false
      thread_(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"), // 创建线程对象，绑定线程函数为threadFunc
      mutex_(),                                                                  // 初始化互斥锁
      cond_(mutex_)
{
} // 初始化条件变量，关联互斥锁

EventLoopThread::~EventLoopThread()
{
  exiting_ = true; // 设置退出标志为true
  if (loop_ != NULL)
  {                 // 如果事件循环对象指针不为NULL
    loop_->quit();  // 退出事件循环
    thread_.join(); // 等待线程结束
  }
}

EventLoop *EventLoopThread::startLoop()
{
  assert(!thread_.started()); // 断言线程未启动
  thread_.start();            // 启动线程 --线程启动后会创建事件对象

  {
    MutexLockGuard lock(mutex_); // 加锁保护临界区
    // 等待---直到事件循环对象被创建并开始运行（运行之后loop_才有真正的指向）
    while (loop_ == NULL)
      cond_.wait();
  }

  return loop_; // 返回事件循环对象指针
}

void EventLoopThread::threadFunc()
{
  EventLoop loop; // 创建事件循环对象

  {
    MutexLockGuard lock(mutex_); // 加锁保护临界区
    loop_ = &loop;               // 将事件循环对象指针指向当前创建的事件循环
    cond_.notify();              // 通知等待的线程，事件循环对象已经创建
  }

  loop.loop(); // 执行指运行事件循环

  loop_ = NULL; // 事件循环结束，置空事件循环对象指针
}
