#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

// 构造函数
AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),                                  // 刷新间隔时间
      running_(false),                                                // 日志记录是否在运行
      basename_(logFileName_),                                        // 日志文件的基础名称
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"), // 创建日志线程
      mutex_(),                                                       // 互斥锁
      cond_(mutex_),                                                  // 条件变量，关联到互斥锁
      currentBuffer_(new Buffer),                                     // 当前缓冲区
      nextBuffer_(new Buffer),                                        // 下一个缓冲区
      buffers_(),                                                     // 缓冲区队列
      latch_(1)
{                                  // 倒计时锁存器
  assert(logFileName_.size() > 1); // 断言日志文件名长度大于1
  currentBuffer_->bzero();         // 清空当前缓冲区
  nextBuffer_->bzero();            // 清空下一个缓冲区
  buffers_.reserve(16);            // 预留缓冲区队列的空间
}

// 追加日志内容到缓冲区
void AsyncLogging::append(const char *logline, int len)
{
  MutexLockGuard lock(mutex_);            // 加锁
  if (currentBuffer_->avail() > len)      // 如果当前缓冲区有足够空间
    currentBuffer_->append(logline, len); // 追加日志内容
  else
  {
    buffers_.push_back(currentBuffer_); // 将当前缓冲区放入队列
    currentBuffer_.reset();             // 重置当前缓冲区指针
    if (nextBuffer_)
      currentBuffer_ = std::move(nextBuffer_); // 使用下一个缓冲区
    else
      currentBuffer_.reset(new Buffer);   // 创建新的缓冲区
    currentBuffer_->append(logline, len); // 追加日志内容
    cond_.notify();                       // 通知日志线程
  }
}

// 日志线程函数
void AsyncLogging::threadFunc()
{
  assert(running_ == true);         // 日志记录在运行
  latch_.countDown();               // 倒计时锁存器减1
  LogFile output(basename_);        // 创建日志文件输出对象
  BufferPtr newBuffer1(new Buffer); // 创建新的缓冲区
  BufferPtr newBuffer2(new Buffer); // 创建另一个新的缓冲区
  newBuffer1->bzero();              // 清空新的缓冲区
  newBuffer2->bzero();              // 清空另一个新的缓冲区
  BufferVector buffersToWrite;      // 写入用的缓冲区队列
  buffersToWrite.reserve(16);       // 预留缓冲区队列的空间
  while (running_)
  {                                                  // 日志记录在运行时
    assert(newBuffer1 && newBuffer1->length() == 0); // 断言新的缓冲区1有效且为空
    assert(newBuffer2 && newBuffer2->length() == 0); // 断言新的缓冲区2有效且为空
    assert(buffersToWrite.empty());                  // 断言写入用的缓冲区队列为空

    {
      MutexLockGuard lock(mutex_); // 加锁
      if (buffers_.empty())
      {                                       // 如果缓冲区队列为空
        cond_.waitForSeconds(flushInterval_); // 等待指定的刷新间隔时间，//每隔3s，或者currentBuffer满了，就将currentBuffer放⼊buffers_中
      }
      buffers_.push_back(currentBuffer_); // 将当前缓冲区放入队列
      currentBuffer_.reset();             // 重置当前缓冲区指针

      currentBuffer_ = std::move(newBuffer1); // 使用新的缓冲区1
      buffersToWrite.swap(buffers_);          // 交换缓冲区队列
      if (!nextBuffer_)
      {
        nextBuffer_ = std::move(newBuffer2); // 使用新的缓冲区2
      }
    }

    assert(!buffersToWrite.empty()); // 断言写入用的缓冲区队列不为空

    if (buffersToWrite.size() > 25)
    {
      // 如果写入用的缓冲区队列大小超过25，丢弃部分缓冲区
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i)
    {
      // 写入每个缓冲区的内容到日志文件
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
    }

    if (buffersToWrite.size() > 2)
    {
      // 丢弃非空的缓冲区，避免浪费
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.back(); // 使用队列最后一个缓冲区
      buffersToWrite.pop_back();
      newBuffer1->reset(); // 重置缓冲区
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.back(); // 使用队列最后一个缓冲区
      buffersToWrite.pop_back();
      newBuffer2->reset(); // 重置缓冲区
    }

    buffersToWrite.clear(); // 清空写入用的缓冲区队列
    output.flush();         // 刷新输出
  }
  output.flush(); // 刷新输出
}
