#include "Thread.h"
#include <assert.h>        // 用于断言
#include <errno.h>         // 用于错误码
#include <linux/unistd.h>  // 用于系统调用号
#include <stdint.h>        // 用于标准整型定义
#include <stdio.h>         // 用于标准输入输出
#include <sys/prctl.h>     // 用于进程控制
#include <sys/types.h>     // 用于基本系统数据类型
#include <unistd.h>        // 用于系统调用
#include <memory>          // 用于智能指针
#include "CurrentThread.h" // 当前线程工具

using namespace std; // 使用标准命名空间

// 当前线程相关变量的定义   每个创建出来的线程都会访问这个命名空间，所以每个线程都会创建一份副本在自己的内存中用于保存线程信息
namespace CurrentThread
{
  __thread int t_cachedTid = 0;                  // 缓存的线程ID
  __thread char t_tidString[32];                 // 线程ID字符串
  __thread int t_tidStringLength = 6;            // 线程ID字符串长度
  __thread const char *t_threadName = "default"; // 线程名称
}

// 获取当前线程ID的函数，使用系统调用获取线程ID
pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

// 缓存当前线程ID的函数
void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {                         // 如果缓存的线程ID为0
    t_cachedTid = gettid(); // 获取当前线程ID
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid); // 将线程ID格式化为字符串
  }
}

// 用于在线程中保留name, tid这些数据的结构体
struct ThreadData
{
  typedef Thread::ThreadFunc ThreadFunc; // 定义线程函数类型
  ThreadFunc func_;                      // 线程函数
  string name_;                          // 线程名称
  pid_t *tid_;                           // 线程ID指针
  CountDownLatch *latch_;                // 倒计时锁存器

  // 构造函数，初始化成员变量
  ThreadData(const ThreadFunc &func, const string &name, pid_t *tid,
             CountDownLatch *latch)
      : func_(func), name_(name), tid_(tid), latch_(latch) {}

  // 在线程中运行的函数
  void runInThread()
  {
    *tid_ = CurrentThread::tid(); // 设置线程ID
    tid_ = NULL;                  // 清空线程ID指针
    latch_->countDown();          // 倒计时锁存器减1
    latch_ = NULL;                // 清空倒计时锁存器指针

    CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str(); // 设置当前线程名称
    prctl(PR_SET_NAME, CurrentThread::t_threadName);                        // 设置线程名称

    func_();                                  // 执行线程函数 ，执行的函数中会创建该线程对应的事件对象
    CurrentThread::t_threadName = "finished"; // 线程名称设置为finished
  }
};

// 线程启动函数
void *startThread(void *obj)
{
  ThreadData *data = static_cast<ThreadData *>(obj); // 将参数转换为ThreadData指针
  data->runInThread();                               // 在线程中运行
  delete data;                                       // 删除ThreadData对象
  return NULL;                                       // 返回空指针
}




// Thread类的构造函数
Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false), // 初始化为未启动
      joined_(false),  // 初始化为未连接
      pthreadId_(0),   // 初始化线程ID为0
      tid_(0),         // 初始化线程ID为0
      func_(func),     // 初始化线程函数
      name_(n),        // 初始化线程名称
      latch_(1)
{                   // 初始化倒计时锁存器为1
  setDefaultName(); // 设置默认线程名称
}

// Thread类的析构函数
Thread::~Thread()
{
  if (started_ && !joined_)
    pthread_detach(pthreadId_); // 如果线程已启动但未连接，则分离线程
}

// 设置默认线程名称
void Thread::setDefaultName()
{
  if (name_.empty())
  { // 如果线程名称为空
    char buf[32];
    snprintf(buf, sizeof buf, "Thread"); // 设置默认名称为"Thread"
    name_ = buf;
  }
}

// 启动线程
void Thread::start()
{
  assert(!started_);                                               // 断言线程未启动
  started_ = true;                                                 // 标记线程已启动
  ThreadData *data = new ThreadData(func_, name_, &tid_, &latch_); // 创建ThreadData对象
  if (pthread_create(&pthreadId_, NULL, &startThread, data))
  {                   // 创建线程
    started_ = false; // 如果创建线程失败，标记线程未启动
    delete data;      // 删除ThreadData对象
  }
  else
  {
    latch_.wait();    // 等待倒计时锁存器
    assert(tid_ > 0); // 断言线程ID大于0
  }
}

// 等待线程结束
int Thread::join()
{
  assert(started_);                      // 断言线程已启动
  assert(!joined_);                      // 断言线程未连接
  joined_ = true;                        // 标记线程已连接
  return pthread_join(pthreadId_, NULL); // 等待线程结束
}
