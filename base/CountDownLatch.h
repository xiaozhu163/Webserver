#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
class CountDownLatch : noncopyable
{
public:
  // 构造函数，初始化count_
  explicit CountDownLatch(int count);

  // 阻塞调用线程，直到count_变为0
  void wait();

  // 递减计数器，若计数器为0则唤醒所有等待的线程
  void countDown();

private:
  mutable MutexLock mutex_; // 互斥锁，用于保护count_
  Condition condition_;     // 条件变量，用于线程间同步
  int count_;               // 计数器
};
