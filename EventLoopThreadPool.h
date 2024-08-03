#pragma once

#include <memory>             // 使用 std::shared_ptr
#include <vector>             // 使用 std::vector
#include "EventLoopThread.h"  // 包含 EventLoopThread 头文件
#include "base/Logging.h"     // 日志记录工具
#include "base/noncopyable.h" // noncopyable 基类

class EventLoopThreadPool : noncopyable
{
public:
  EventLoopThreadPool(EventLoop *baseLoop, int numThreads); // 构造函数，初始化线程池

  ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; } // 析构函数，打印析构日志

  void start(); // 启动线程池中的所有线程

  EventLoop *getNextLoop(); // 获取下一个可用的 EventLoop 对象

private:
  EventLoop *baseLoop_;                                   // 基础 EventLoop 对象指针
  bool started_;                                          // 标志，表示线程池是否已启动
  int numThreads_;                                        // 线程池中的线程数量
  int next_;                                              // 下一个要使用的线程索引
  std::vector<std::shared_ptr<EventLoopThread>> threads_; // EventLoopThread 实例的共享指针容器
  std::vector<EventLoop *> loops_;                        // EventLoop 对象的容器
};
