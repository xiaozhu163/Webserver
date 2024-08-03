#include "Epoll.h"
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <deque>
#include <queue>
#include "Util.h"
#include "base/Logging.h"

#include <arpa/inet.h>
#include <iostream>
using namespace std;

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

typedef shared_ptr<Channel> SP_Channel;

Epoll::Epoll()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)), // 创建 epoll 实例，并设置 close-on-exec 标志
      events_(EVENTSNUM)                      // 初始化事件列表大小
{
  // 确保 epoll 文件描述符创建成功
  assert(epollFd_ > 0);
}

Epoll::~Epoll()
{
  // 这里可以添加清理资源的代码，如果有需要
}

// 注册新描述符
void Epoll::epoll_add(SP_Channel request, int timeout)
{
  int fd = request->getFd(); // 获取请求对应的文件描述符

  if (timeout > 0) // 如果设置了超时时间，添加定时器
  {
    //每次增加一个新的事件时添加一个定时器节点，这时一个事件第一次添加定时器
    add_timer(request, timeout);         // 为该请求添加定时器
    fd2http_[fd] = request->getHolder(); // 将文件描述符与HttpData对象的持有者进行关联
  }
  struct epoll_event event;            // 创建epoll事件
  event.data.fd = fd;                  // 设置事件关联的文件描述符
  event.events = request->getEvents(); // 设置事件类型
  request->EqualAndUpdateLastEvents(); // 更新请求的最后事件
  fd2chan_[fd] = request;              // 将文件描述符与Channel对象进行关联

  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) // 向epoll实例中添加新的事件监听
  {
    perror("epoll_add error"); // 如果epoll_ctl添加事件失败，输出错误信息
    fd2chan_[fd].reset();      // 重置关联的Channel对象
  }
}

// 修改描述符状态
void Epoll::epoll_mod(SP_Channel request, int timeout)
{
  if (timeout > 0)
  //每次修改一个 通道/连接 对应的监听事件时添加一个定时器节点
    add_timer(request, timeout);
  int fd = request->getFd();
  if (!request->EqualAndUpdateLastEvents())
  {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
    {
      perror("epoll_mod error");
      fd2chan_[fd].reset();
    }
  }
}

// 从epoll中删除描述符
void Epoll::epoll_del(SP_Channel request)
{
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  // event.events = 0;
  // request->EqualAndUpdateLastEvents()
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
  {
    perror("epoll_del error");
  }
  fd2chan_[fd].reset();
  fd2http_[fd].reset();
}

// 返回活跃事件数
std::vector<SP_Channel> Epoll::poll()
{
  while (true)
  {
    int event_count =
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0)
      perror("epoll wait error");
    std::vector<SP_Channel> req_data = getEventsRequest(event_count);
    if (req_data.size() > 0)
      return req_data;
  }
}

void Epoll::handleExpired() { timerManager_.handleExpiredEvent(); }

// 分发处理函数
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num)
{
  std::vector<SP_Channel> req_data;
  for (int i = 0; i < events_num; ++i)
  {
    // 获取有事件产生的描述符
    int fd = events_[i].data.fd;

    SP_Channel cur_req = fd2chan_[fd];

    if (cur_req)
    {
      cur_req->setRevents(events_[i].events);
      cur_req->setEvents(0);
      // 加入线程池之前将Timer和request分离
      // cur_req->seperateTimer();
      req_data.push_back(cur_req);
    }
    else
    {
      LOG << "SP cur_req is invalid";
    }
  }
  return req_data;
}

//在出现读写事件或者其他事件时，不会更新现有定时器，因为定时器是个队列，只会添加定时器，并修改当前httpdata对应的定时器指针的指向
void Epoll::add_timer(SP_Channel request_data, int timeout)
{
  shared_ptr<HttpData> t = request_data->getHolder();
  if (t)
    timerManager_.addTimer(t, timeout);
  else
    LOG << "timer add fail";
}