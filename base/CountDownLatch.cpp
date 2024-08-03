#include "CountDownLatch.h"

// 构造函数，初始化成员变量
CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

// 阻塞调用线程，直到count_变为0
void CountDownLatch::wait()
{
  // 加锁，保护共享变量
  MutexLockGuard lock(mutex_);

  // 如果count_大于0，则等待条件变量的通知
  while (count_ > 0)
    condition_.wait();
}

// 递减计数器，若计数器为0则唤醒所有等待的线程
void CountDownLatch::countDown()
{
  // 加锁，保护共享变量
  MutexLockGuard lock(mutex_);

  // 递减计数器
  --count_;

  // 如果计数器为0，通知所有等待的线程
  if (count_ == 0)
    condition_.notifyAll();
}
