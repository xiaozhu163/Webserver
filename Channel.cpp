#include "Channel.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <queue>

#include "Epoll.h"
#include "EventLoop.h"
#include "Util.h"

using namespace std;

// 默认构造函数，初始化Channel对象
Channel::Channel(EventLoop *loop)
    : loop_(loop), events_(0), lastEvents_(0), fd_(0) {}

// 带参构造函数，初始化Channel对象，并设置文件描述符fd
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), lastEvents_(0) {}

// 析构函数，释放资源（已注释掉的部分）
Channel::~Channel()
{
  // loop_->poller_->epoll_del(fd, events_);
  // close(fd_);
}

// 获取Channel的文件描述符
int Channel::getFd() { return fd_; }

// 设置Channel的文件描述符
void Channel::setFd(int fd) { fd_ = fd; }

// 处理读事件的回调函数
void Channel::handleRead()
{
  if (readHandler_)
  {
    readHandler_(); // 调用注册的读事件处理器
  }
}

// 处理写事件的回调函数
void Channel::handleWrite()
{
  if (writeHandler_)
  {
    writeHandler_(); // 调用注册的写事件处理器
  }
}

// 处理连接事件的回调函数
void Channel::handleConn()
{
  if (connHandler_)
  {
    connHandler_(); // 调用注册的连接事件处理器
  }
}
