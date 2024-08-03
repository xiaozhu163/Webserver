#pragma once
#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"

/*
MutexLock 类：
  * 实现了互斥锁的基本功能，包括构造函数、析构函数、加锁和解锁操作。
  * 使用了 pthread_mutex_t 类型来表示互斥锁变量。
  * 构造函数初始化互斥锁，析构函数销毁互斥锁。
  * 提供了 lock() 和 unlock() 方法用于加锁和解锁操作。
  * get() 方法用于获取互斥锁的指针。
  * 将 MutexLock 声明为 noncopyable 类，确保该类的对象不能被复制或赋值。
 */

class MutexLock : noncopyable
{
public:
  MutexLock() { pthread_mutex_init(&mutex, NULL); } // 构造函数，初始化互斥锁

  // 析构函数，销毁互斥锁
  ~MutexLock()
  {
    pthread_mutex_lock(&mutex);
    pthread_mutex_destroy(&mutex);
  }

  void lock() { pthread_mutex_lock(&mutex); }     // 加锁操作
  void unlock() { pthread_mutex_unlock(&mutex); } // 解锁操作
  pthread_mutex_t *get() { return &mutex; }       // 获取互斥锁的指针

private:
  pthread_mutex_t mutex; // 互斥锁变量

  // 友元类不受访问权限影响
private:
  friend class Condition; // 声明 Condition 类为友元类
};

/*
MutexLockGuard 类：
  * 用于自动管理互斥锁的加锁和解锁操作，利用了 RAII（资源获取即初始化）的原理。
  * 在构造函数中对给定的互斥锁对象进行加锁操作，在析构函数中进行解锁操作，从而保证了在对象生命周期结束时自动释放锁。
  * 将 MutexLockGuard 声明为 noncopyable 类，确保该类的对象不能被复制或赋值。
 */

class MutexLockGuard : noncopyable
{
public:
  explicit MutexLockGuard(MutexLock &_mutex) : mutex(_mutex) { mutex.lock(); } // 显式构造函数，对给定的互斥锁对象进行加锁操作

  ~MutexLockGuard() { mutex.unlock(); } // 析构函数，进行解锁操作

private:
  MutexLock &mutex; // 互斥锁的引用
};
