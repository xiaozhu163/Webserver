#pragma once

#include <assert.h>
#include <string.h>
#include <string>
#include "noncopyable.h" // 非拷贝able类的头文件，用于禁止拷贝构造和赋值

class AsyncLogging; // 前向声明AsyncLogging类

const int kSmallBuffer = 4000;        // 小缓冲区大小常量
const int kLargeBuffer = 4000 * 1000; // 大缓冲区大小常量

// 定义一个固定大小的缓冲区
template <int SIZE>
class FixedBuffer : noncopyable // 固定大小缓冲区模板类，继承自noncopyable
{
public:
  FixedBuffer() : cur_(data_) {} // 构造函数，初始化当前指针为data_

  ~FixedBuffer() {} // 析构函数，无特殊操作

  void append(const char *buf, size_t len)
  { // 添加数据到缓冲区
    if (avail() > static_cast<int>(len))
    {                         // 如果剩余空间足够
      memcpy(cur_, buf, len); // 将数据复制到缓冲区
      cur_ += len;            // 更新当前指针位置
    }
  }

  const char *data() const { return data_; }                    // 返回缓冲区数据起始指针
  int length() const { return static_cast<int>(cur_ - data_); } // 返回缓冲区当前已使用长度

  char *current() { return cur_; }                             // 返回当前指针位置
  int avail() const { return static_cast<int>(end() - cur_); } // 返回缓冲区剩余可用空间
  void add(size_t len) { cur_ += len; }                        // 增加当前指针位置

  void reset() { cur_ = data_; }                   // 重置当前指针位置为缓冲区起始位置
  void bzero() { memset(data_, 0, sizeof data_); } // 将整个缓冲区清零

private:
  const char *end() const { return data_ + sizeof data_; } // 返回缓冲区结束位置

  char data_[SIZE]; // 缓冲区数组
  char *cur_;       // 当前指针位置
};

// 日志流类，继承自noncopyable
class LogStream : noncopyable
{
public:
  typedef FixedBuffer<kSmallBuffer> Buffer; // 使用FixedBuffer模板定义Buffer类型

  LogStream &operator<<(bool v)
  {                                   // bool类型输出操作符重载
    buffer_.append(v ? "1" : "0", 1); // 将bool值转换为字符'1'或'0'，添加到buffer中
    return *this;
  }

  LogStream &operator<<(short);              // 声明short类型的输出操作符函数
  LogStream &operator<<(unsigned short);     // 声明unsigned short类型的输出操作符函数
  LogStream &operator<<(int);                // 声明int类型的输出操作符函数
  LogStream &operator<<(unsigned int);       // 声明unsigned int类型的输出操作符函数
  LogStream &operator<<(long);               // 声明long类型的输出操作符函数
  LogStream &operator<<(unsigned long);      // 声明unsigned long类型的输出操作符函数
  LogStream &operator<<(long long);          // 声明long long类型的输出操作符函数
  LogStream &operator<<(unsigned long long); // 声明unsigned long long类型的输出操作符函数

  LogStream &operator<<(const void *); // 声明指针类型的输出操作符函数

  LogStream &operator<<(float v)
  {                                  // float类型输出操作符重载
    *this << static_cast<double>(v); // 将float类型转换为double类型，再进行输出
    return *this;
  }
  LogStream &operator<<(double);      // 声明double类型的输出操作符函数
  LogStream &operator<<(long double); // 声明long double类型的输出操作符函数

  LogStream &operator<<(char v)
  {                        // char类型输出操作符重载
    buffer_.append(&v, 1); // 将单个字符添加到buffer中
    return *this;
  }

  LogStream &operator<<(const char *str)
  { // C风格字符串输出操作符重载
    if (str)
      buffer_.append(str, strlen(str)); // 将C风格字符串添加到buffer中
    else
      buffer_.append("(null)", 6); // 如果为空指针，则添加"(null)"到buffer中
    return *this;
  }

  LogStream &operator<<(const unsigned char *str)
  {                                                         // 无符号字符指针输出操作符重载
    return operator<<(reinterpret_cast<const char *>(str)); // 输出无符号字符指针的操作符重载
  }

  LogStream &operator<<(const std::string &v)
  {                                      // std::string类型输出操作符重载
    buffer_.append(v.c_str(), v.size()); // 将std::string对象添加到buffer中
    return *this;
  }

  void append(const char *data, int len) { buffer_.append(data, len); } // 添加指定长度的字符数据到buffer中
  const Buffer &buffer() const { return buffer_; }                      // 获取buffer对象的引用
  void resetBuffer() { buffer_.reset(); }                               // 重置buffer对象

private:
  void staticCheck(); // 静态检查方法声明

  template <typename T>
  void formatInteger(T); // 格式化整数类型数据的模板方法声明

  Buffer buffer_; // 日志缓冲区对象

  static const int kMaxNumericSize = 32; // 最大数字长度常量
};
