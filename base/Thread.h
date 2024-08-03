#pragma once
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <string>
#include "CountDownLatch.h"
#include "noncopyable.h"

/*
Thread 类：
  * 该类封装了一个线程的基本功能，包括启动、等待线程结束等操作。
  * 提供了线程函数的设置和线程名称的管理。
  * 使用 pthread 库进行线程的创建和管理。
  * 使用了 CountDownLatch 类来确保线程启动时的同步。
  * 将 Thread 类声明为 noncopyable，确保该类的对象不能被复制或赋值。
*/

class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc; // 定义线程函数类型

    // 构造函数，接受线程函数和线程名称作为参数
    explicit Thread(const ThreadFunc &func, const std::string &name = std::string());

    // 析构函数
    ~Thread();

    // 启动线程
    void start();

    // 等待线程结束
    int join();

    // 返回线程是否已启动
    bool started() const { return started_; }

    // 返回线程的 tid
    pid_t tid() const { return tid_; }

    // 返回线程的名称
    const std::string &name() const { return name_; }

private:
    // 设置默认的线程名称
    void setDefaultName();

    bool started_;         // 线程是否已启动
    bool joined_;          // 线程是否已结束
    pthread_t pthreadId_;  // 线程的 pthread ID
    pid_t tid_;            // 线程的系统 tid
    ThreadFunc func_;      // 线程函数
    std::string name_;     // 线程名称
    CountDownLatch latch_; // 倒计时锁存器，用于线程启动同步
};
