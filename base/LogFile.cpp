#include "LogFile.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "FileUtil.h"

using namespace std;

// 构造函数，初始化日志文件的基本名称、刷新间隔、计数器和互斥锁，并打开日志文件
LogFile::LogFile(const string &basename, int flushEveryN)
    : basename_(basename),       // 初始化日志文件的基本名称
      flushEveryN_(flushEveryN), // 初始化刷新间隔
      count_(0),                 // 初始化写入计数器
      mutex_(new MutexLock)      // 创建互斥锁对象
{
  // assert(basename.find('/') >= 0);   // 确保日志文件名中包含路径（可选）
  file_.reset(new AppendFile(basename)); // 打开日志文件
}

// 析构函数
LogFile::~LogFile() {}

// 追加日志内容，加锁保护，确保线程安全
void LogFile::append(const char *logline, int len)
{
  MutexLockGuard lock(*mutex_);  // 加锁保护
  append_unlocked(logline, len); // 追加日志内容（内部实现）
}

// 刷新日志文件，加锁保护，确保线程安全
void LogFile::flush()
{
  MutexLockGuard lock(*mutex_); // 加锁保护
  file_->flush();               // 刷新日志文件
}

// 追加日志内容的内部实现（不加锁版本）
void LogFile::append_unlocked(const char *logline, int len)
{
  file_->append(logline, len); // 将日志内容追加到文件
  ++count_;                    // 增加写入计数器
  if (count_ >= flushEveryN_)
  {                 // 如果达到刷新间隔
    count_ = 0;     // 重置计数器
    file_->flush(); // 刷新日志文件
  }
}
