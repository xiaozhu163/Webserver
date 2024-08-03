#pragma once
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include "MutexLock.h"
#include "noncopyable.h"

// Condition类封装了pthread条件变量的操作，用于线程间的同步
class Condition : noncopyable {
 public:
  // 构造函数，初始化条件变量并与互斥锁关联
  explicit Condition(MutexLock &_mutex) : mutex(_mutex) {
    pthread_cond_init(&cond, NULL);
  }

  // 析构函数，销毁条件变量
  ~Condition() { pthread_cond_destroy(&cond); }

  // 等待条件变量，当前线程阻塞并释放锁，直到被通知
  void wait() { pthread_cond_wait(&cond, mutex.get()); }

  // 通知一个等待条件变量的线程
  void notify() { pthread_cond_signal(&cond); }

  // 通知所有等待条件变量的线程
  void notifyAll() { pthread_cond_broadcast(&cond); }

  // 等待条件变量，当前线程阻塞并释放锁，直到被通知或超时
  bool waitForSeconds(int seconds) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime); // 获取当前时间
    abstime.tv_sec += static_cast<time_t>(seconds); // 设置等待的绝对时间
    return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime); // 超时返回true
  }

 private:
  MutexLock &mutex; // 引用关联的互斥锁
  pthread_cond_t cond; // 条件变量
};
