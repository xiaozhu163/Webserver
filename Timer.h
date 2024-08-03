#pragma once

#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include "HttpData.h"
#include "base/MutexLock.h"
#include "base/noncopyable.h"

class HttpData; // 前置声明 HttpData 类

// 定时器节点类，用于管理每个 HTTP 请求的定时器信息
class TimerNode
{
public:
  TimerNode(std::shared_ptr<HttpData> requestData, int timeout); // 构造函数，初始化定时器节点
  ~TimerNode();                                                  // 析构函数，清理资源
  TimerNode(TimerNode &tn);                                      // 拷贝构造函数，目前没有具体实现

  void update(int timeout);                          // 更新定时器超时时间
  bool isValid();                                    // 检查定时器节点是否有效
  void clearReq();                                   // 清除与定时器节点关联的 HTTP 请求数据
  void setDeleted() { deleted_ = true; }             // 设置定时器节点为已删除状态
  bool isDeleted() const { return deleted_; }        // 检查定时器节点是否被标记为已删除
  size_t getExpTime() const { return expiredTime_; } // 获取定时器节点的过期时间

private:
  bool deleted_;                        // 标记定时器节点是否被删除
  size_t expiredTime_;                  // 定时器节点的过期时间
  std::shared_ptr<HttpData> SPHttpData; // 指向 HTTP 数据的智能指针
};

// 比较器结构体，用于优先队列中定时器节点的比较
struct TimerCmp
{
  bool operator()(std::shared_ptr<TimerNode> &a,
                  std::shared_ptr<TimerNode> &b) const
  {
    // 按照定时器节点的过期时间从小到大排序
    return a->getExpTime() > b->getExpTime();
  }
};

// 定时器管理类，负责管理所有的定时器节点
// 每个epoll都会有一个定时器管理器，用来管理其管理的所有事件添加进去的定时器队列
class TimerManager
{
public:
  TimerManager();  // 构造函数，初始化定时器管理器
  ~TimerManager(); // 析构函数，清理资源

  void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout); // 向定时器管理器中添加一个定时器节点
  void handleExpiredEvent();                                        // 处理已经过期的定时器事件

private:
  typedef std::shared_ptr<TimerNode> SPTimerNode;
  std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue; // 定时器节点优先队列
};
