#pragma once

#include <memory>
#include <string>
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"

// LogFile类提供日志文件的写入功能，并实现自动刷新和文件滚动功能
class LogFile : noncopyable
{
public:
  // 每被append flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
  LogFile(const std::string &basename, int flushEveryN = 1024); // 构造函数，接受日志文件的基本名称和每次刷新所需的写入次数
  ~LogFile();                                                   // 析构函数

  void append(const char *logline, int len); // 追加日志内容，接受日志内容和长度
  void flush();                              // 刷新日志文件，将缓冲区内容写入文件
  bool rollFile();                           // 滚动日志文件，判断是否需要创建新的日志文件

private:
  void append_unlocked(const char *logline, int len); // 追加日志内容（不加锁版本）

  const std::string basename_; // 日志文件的基本名称
  const int flushEveryN_;      // 每次刷新所需的写入次数

  int count_;                        // 当前的写入次数计数
  std::unique_ptr<MutexLock> mutex_; // 互斥锁，用于保护文件写入操作
  std::unique_ptr<AppendFile> file_; // 日志文件对象，负责文件写入操作
};
