#pragma once
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"

// Epoll类用于封装epoll相关操作
class Epoll
{
public:
  Epoll();
  ~Epoll();

  void epoll_add(SP_Channel request, int timeout); // 向epoll实例中添加文件描述符及其关联的事件
  void epoll_mod(SP_Channel request, int timeout); // 修改epoll实例中已存在的文件描述符及其关联的事件
  void epoll_del(SP_Channel request); // 从epoll实例中删除文件描述符及其关联的事件
  std::vector<std::shared_ptr<Channel>> poll(); // 等待事件的发生，并返回活动的Channel列表
  std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num); // 获取已触发事件的Channel列表
  void add_timer(std::shared_ptr<Channel> request_data, int timeout); // 为指定的Channel添加定时器
  int getEpollFd() { return epollFd_; } // 获取epoll实例的文件描述符
  void handleExpired(); // 处理已过期的定时器

private:
  static const int MAXFDS = 100000; // 最大文件描述符数
  int epollFd_; // epoll实例的文件描述符，通过epoll_create创建
  std::vector<epoll_event> events_; // 存放epoll_wait返回的活动事件
  std::shared_ptr<Channel> fd2chan_[MAXFDS]; // 文件描述符到Channel的映射表
  std::shared_ptr<HttpData> fd2http_[MAXFDS]; // 文件描述符到HttpData的映射表
  TimerManager timerManager_; // 定时器管理器
};
