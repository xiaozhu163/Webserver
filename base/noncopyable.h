#pragma once

// 该类禁止对象复制和赋值
class noncopyable {
 protected:
  // 受保护的默认构造函数，允许派生类构造对象
  noncopyable() {}
  // 受保护的析构函数，允许派生类析构对象
  ~noncopyable() {}

 private:
  // 私有的拷贝构造函数，禁止对象的复制
  noncopyable(const noncopyable&);
  // 私有的拷贝赋值运算符，禁止对象的赋值
  const noncopyable& operator=(const noncopyable&);
};
