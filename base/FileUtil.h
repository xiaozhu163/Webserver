#pragma once
#include <string>
#include "noncopyable.h"

// AppendFile类，提供文件追加和刷新功能，继承自noncopyable以禁止拷贝操作
class AppendFile : noncopyable
{
public:
  // 显式构造函数，接受文件名参数
  explicit AppendFile(std::string filename);

  // 析构函数，负责清理资源
  ~AppendFile();

  // 追加日志内容到文件
  void append(const char *logline, const size_t len);

  // 刷新文件缓冲区到磁盘
  void flush();

private:
  // 写入日志内容到文件，返回实际写入的字节数
  size_t write(const char *logline, size_t len);

  FILE *fp_;               // 文件指针，用于文件操作
  char buffer_[64 * 1024]; // 文件写入缓冲区，大小为64KB
};
