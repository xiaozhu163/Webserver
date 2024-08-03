#pragma once
#include <stdint.h>

namespace CurrentThread
{
  // 线程局部存储的变量，用于缓存线程ID及其相关信息
  extern __thread int t_cachedTid;          // 缓存的线程ID
  extern __thread char t_tidString[32];     // 线程ID的字符串表示
  extern __thread int t_tidStringLength;    // 线程ID字符串的长度
  extern __thread const char *t_threadName; // 线程名称

  // 缓存当前线程ID的函数声明
  void cacheTid();

  // 返回当前线程ID的函数，如果未缓存则调用cacheTid()进行缓存
  inline int tid()
  {
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid(); // 如果线程ID未缓存，则调用cacheTid()缓存线程ID
    }
    return t_cachedTid; // 返回线程ID
  }

  // 返回线程ID的字符串表示，用于日志记录
  inline const char *tidString()
  {
    return t_tidString; // 返回线程ID的字符串表示
  }

  // 返回线程ID字符串的长度，用于日志记录
  inline int tidStringLength()
  {
    return t_tidStringLength; // 返回线程ID字符串的长度
  }

  // 返回线程名称
  inline const char *name()
  {
    return t_threadName; // 返回当前线程的名称
  }
}
