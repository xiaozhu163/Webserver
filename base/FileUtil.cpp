#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

// AppendFile构造函数，接受文件名作为参数
AppendFile::AppendFile(string filename)
    : fp_(fopen(filename.c_str(), "ae")) // 以附加模式打开文件，文件指针指向文件末尾
{
  setbuffer(fp_, buffer_, sizeof buffer_); // 用户提供缓冲区，将自定义缓冲区与文件流相关联
}

// AppendFile析构函数，负责关闭文件指针
AppendFile::~AppendFile()
{
  fclose(fp_); // 关闭文件
}

// 追加日志内容到文件
void AppendFile::append(const char *logline, const size_t len)
{
  size_t n = this->write(logline, len); // 写入日志内容，返回写入的字节数
  size_t remain = len - n;              // 计算剩余未写入的字节数
  while (remain > 0)
  {                                              // 当仍有未写入的数据时，继续写入
    size_t x = this->write(logline + n, remain); // 写入剩余数据
    if (x == 0)
    {                        // 如果写入字节数为0，表示写入失败
      int err = ferror(fp_); // 获取文件错误状态
      if (err)
        fprintf(stderr, "AppendFile::append() failed !\n"); // 输出错误信息
      break;                                                // 退出循环
    }
    n += x;           // 更新已写入字节数
    remain = len - n; // 更新剩余未写入的字节数
  }
}

// 刷新文件缓冲区到磁盘
void AppendFile::flush()
{
  fflush(fp_); // 刷新文件流，将缓冲区内容写入文件
}

// 写入日志内容到文件，返回实际写入的字节数
size_t AppendFile::write(const char *logline, size_t len)
{
  return fwrite_unlocked(logline, 1, len, fp_); // 使用fwrite_unlocked提高写入性能
}
