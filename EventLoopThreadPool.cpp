#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0)
{
  if (numThreads_ <= 0)
  {
    LOG << "numThreads_ <= 0"; // 记录日志，线程数量小于等于0异常
    abort();                   // 终止程序
  }
}

void EventLoopThreadPool::start()
{
  baseLoop_->assertInLoopThread(); // 断言在创建线程池的线程中调用（主线程）
  started_ = true;                 // 标记线程池已启动
  for (int i = 0; i < numThreads_; ++i)
  {
    std::shared_ptr<EventLoopThread> t(new EventLoopThread()); // 创建 EventLoopThread 对象的共享指针
    threads_.push_back(t);                                     // 将指针添加到线程容器中
    loops_.push_back(t->startLoop());                          // 获取并存储线程中的 EventLoop 对象
  }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread(); // 断言在创建线程池的线程中调用（主线程）
  assert(started_);                // 断言线程池已经启动
  EventLoop *loop = baseLoop_;     // 默认使用基础 EventLoop（主线程对应的eventloop）
  if (!loops_.empty())
  {
    loop = loops_[next_];              // 获取下一个可用的 EventLoop
    next_ = (next_ + 1) % numThreads_; // 更新下一个要使用的索引
  }
  return loop;
}
