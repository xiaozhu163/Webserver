#pragma once

#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"

/*
 * 异步日志类
 * 该类负责将前端获得的日志数据放入后端的缓冲区，并定期将缓冲区内容写入磁盘。
 */
class AsyncLogging : noncopyable
{
public:
  // 构造函数，接受日志文件的基础名称和刷新间隔（默认为2秒）
  AsyncLogging(const std::string basename, int flushInterval = 2);

  // 析构函数，确保线程已停止
  ~AsyncLogging()
  {
    if (running_)
      stop();
  }

  // 向日志系统追加日志数据
  void append(const char *logline, int len);

  // 启动日志线程，初始化并等待线程启动完成
  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  // 停止日志线程，唤醒线程并等待其结束
  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

private:
  // 线程执行函数，不断将日志从前端缓冲区写入后端的缓冲区或者磁盘
  void threadFunc();

  typedef FixedBuffer<kLargeBuffer> Buffer;                  // 定义使用较大缓冲区的日志缓冲类型，在stream中定义的smallbuffer是较小的缓冲区
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector; // 日志缓冲区的容器类型
  typedef std::shared_ptr<Buffer> BufferPtr;                 // 日志缓冲区指针类型

  const int flushInterval_; // 刷新间隔
  bool running_;            // 运行状态标志
  std::string basename_;    // 日志文件的基础名称
  Thread thread_;           // 日志线程对象，后端线程，⽤于将⽇志写⼊⽂件，后端通过⼀个单独的线程来实现将⽇志写⼊⽂件的，这就是那个线程对象。
  MutexLock mutex_;         // 互斥锁，保护日志缓冲区的访问
  Condition cond_;          // 条件变量，用于线程间同步
  BufferPtr currentBuffer_; // 当前活跃的日志缓冲区
  BufferPtr nextBuffer_;    // 下一个待写入的日志缓冲区  ，，cur和next双缓冲区，减少前端等待的开销
  BufferVector buffers_;    // 日志缓冲区的容器，缓冲区队列，实际写⼊⽂件的内容也是从这⾥拿的
  CountDownLatch latch_;    // 用于等待线程启动完成的计数器
};
