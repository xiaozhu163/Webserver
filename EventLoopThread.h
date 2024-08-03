#pragma once

#include "EventLoop.h"        // 包含EventLoop头文件，定义了事件循环类
#include "base/Condition.h"   // 包含Condition头文件，定义了条件变量类
#include "base/MutexLock.h"   // 包含MutexLock头文件，定义了互斥锁类
#include "base/Thread.h"      // 包含Thread头文件，定义了线程类
#include "base/noncopyable.h" // 包含noncopyable头文件，定义了禁止复制类

class EventLoopThread : noncopyable
{
public:
  EventLoopThread();      // 构造函数
  ~EventLoopThread();     // 析构函数
  EventLoop *startLoop(); // 启动事件循环并返回事件循环对象的指针

private:
  void threadFunc(); // 线程函数，实际执行事件循环的函数
  EventLoop *loop_;  // 指向事件循环对象的指针
  bool exiting_;     // 标志线程是否正在退出
  Thread thread_;    // 线程对象，用于管理事件循环线程的生命周期
  MutexLock mutex_;  // 互斥锁，用于保护条件变量和退出标志
  Condition cond_;   // 条件变量，用于线程间的同步等待
};
