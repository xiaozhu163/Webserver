#include "LogStream.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <limits>

// 数字字符数组，用于数字转换
const char digits[] = "9876543210123456789"; //以0为中心 正负数怕偏移都能得到正确的偏移绝对值对应的字符
const char* zero = digits + 9;

// 从muduo库中借用的转换函数模板
template <typename T>
size_t convert(char buf[], T value) {
  T i = value;
  char* p = buf;

  do {
    int lsd = static_cast<int>(i % 10);  // 获取最低位数字
    i /= 10;  // 更新i，去掉最低位数字
    *p++ = zero[lsd];  // 存储最低位数字字符
  } while (i != 0);  // 循环直到i为0

  if (value < 0) {
    *p++ = '-';  // 如果是负数，添加负号
  }
  *p = '\0';  // 终止字符串
  std::reverse(buf, p);  // 反转字符数组

  return p - buf;  // 返回字符串长度
}

// 显式实例化模板类
template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

// 将整数格式化为字符串，并添加到buffer中
template <typename T>
void LogStream::formatInteger(T v) {
  // 确保buffer有足够空间存储数字字符串
  if (buffer_.avail() >= kMaxNumericSize) {
    size_t len = convert(buffer_.current(), v);  // 转换整数并获取长度
    buffer_.add(len);  // 更新buffer位置
  }
}

// 重载<<操作符，用于各种整数类型
LogStream& LogStream::operator<<(short v) {
  *this << static_cast<int>(v);  // 转换为int并调用重载函数
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v) {
  *this << static_cast<unsigned int>(v);  // 转换为unsigned int并调用重载函数
  return *this;
}

LogStream& LogStream::operator<<(int v) {
  formatInteger(v);  // 格式化整数
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v) {
  formatInteger(v);  // 格式化无符号整数
  return *this;
}

LogStream& LogStream::operator<<(long v) {
  formatInteger(v);  // 格式化长整数
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
  formatInteger(v);  // 格式化无符号长整数
  return *this;
}

LogStream& LogStream::operator<<(long long v) {
  formatInteger(v);  // 格式化长长整数
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
  formatInteger(v);  // 格式化无符号长长整数
  return *this;
}

// 重载<<操作符，用于浮点数类型
LogStream& LogStream::operator<<(double v) {
  if (buffer_.avail() >= kMaxNumericSize) {
    int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);  // 格式化双精度浮点数
    buffer_.add(len);  // 更新buffer位置
  }
  return *this;
}

LogStream& LogStream::operator<<(long double v) {
  if (buffer_.avail() >= kMaxNumericSize) {
    int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);  // 格式化长双精度浮点数
    buffer_.add(len);  // 更新buffer位置
  }
  return *this;
}
