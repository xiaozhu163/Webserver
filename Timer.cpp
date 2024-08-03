#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout)
    : deleted_(false), SPHttpData(requestData)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  // 计算超时时间点（毫秒）
  expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode()
{
  // 如果关联的HttpData对象存在，则关闭它
  if (SPHttpData)
    SPHttpData->handleClose();
}

TimerNode::TimerNode(TimerNode &tn)
    : SPHttpData(tn.SPHttpData), expiredTime_(0) {}

void TimerNode::update(int timeout)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  // 更新超时时间点（毫秒）
  expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid()
{
  struct timeval now;
  gettimeofday(&now, NULL);
  size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
  // 检查当前时间是否小于超时时间点，判断节点是否有效
  if (temp < expiredTime_)
    return true;
  else
  {
    this->setDeleted(); // 如果超时，将节点标记为删除
    return false;
  }
}

void TimerNode::clearReq()
{
  SPHttpData.reset(); // 重置关联的HttpData对象
  this->setDeleted(); // 标记节点为删除状态
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout)
{
  // 创建新的TimerNode并添加到优先队列中
  SPTimerNode new_node(new TimerNode(SPHttpData, timeout));
  timerNodeQueue.push(new_node);
  // 关联HttpData对象与TimerNode
  //每次有读写事件时，会将http_data的定时器指针指向新的定时器节点，而不是更新现有的定时器节点，旧的定时器节点会每次epoll_wait返回时执行完通道对应的事件调用函数后，删除更新一次定时器队列；
  SPHttpData->linkTimer(new_node);
}

/* 定时器事件处理逻辑说明：
因为：
(1) 优先队列不支持随机访问；
(2) 即使支持，随机删除某节点后会破坏堆的结构，需要重新更新堆结构。
所以，对于被置为deleted的时间节点，会延迟到：
(1) 节点超时；
(2) 它前面的节点都被删除时，才会被删除。
一个被置为deleted的节点，最迟会在超时后的TIMER_TIME_OUT时间内被删除。
这样做有两个好处：
(1) 不需要遍历优先队列，提高效率；
(2) 给超时时间一个容忍的时间，设定的超时时间是删除的下限，并非一到超时时间立即删除。
   如果在超时后的下一次请求中，监听的请求再次出现，就不需要重新申请RequestData节点，
   这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的开销。
*/

//每个eventloop在每次处理所有发生的事件之后，会处理一次即httpdata中定义的void HttpData::handleConn()超时事件，遍历每个定时器队列中节点，查看每个定时器是否isValid，即是否超时，如果超时设置delete，然后pop掉
void TimerManager::handleExpiredEvent()
{
  // MutexLockGuard locker(lock);  // 加锁（未实现MutexLock）
  while (!timerNodeQueue.empty())
  {
    SPTimerNode ptimer_now = timerNodeQueue.top();
    if (ptimer_now->isDeleted())
      timerNodeQueue.pop(); // 弹出已标记为删除的节点
    else if (ptimer_now->isValid() == false)
      timerNodeQueue.pop(); // 弹出超时无效的节点
    else
      break; // 遇到有效节点，结束处理
  }
}
