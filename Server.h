#pragma once
#include <memory>                // 用于std::unique_ptr和std::shared_ptr
#include "Channel.h"             // 包含Channel类的定义
#include "EventLoop.h"           // 包含EventLoop类的定义
#include "EventLoopThreadPool.h" // 包含EventLoopThreadPool类的定义
#include "base/MemoryPool.h"     // 包含内存池的声明
#include "CGImysql/sql_connection_pool.h"

extern connection_pool *m_connPool; // 数据库连接池

// Server类用于管理服务器的生命周期，处理新连接和分发事件
class Server
{
public:
  Server(EventLoop *loop, int threadNum, int port, string user, string password, string m_databaseName); // 构造函数，初始化Server对象
  ~Server() {}                                      // 析构函数

  EventLoop *getLoop() const { return loop_; }                 // 获取当前EventLoop
  void start();                                                // 启动服务器
  void handNewConn();                                          // 处理新连接
  void handThisConn() { loop_->updatePoller(acceptChannel_); } // 更新当前连接的poller
  void sql_pool();                                             // 数据库连接池

private:
  EventLoop *loop_;                                          // 主EventLoop
  int threadNum_;                                            // 线程池中的线程数量
  std::unique_ptr<EventLoopThreadPool, DeleteElement<EventLoopThreadPool>> eventLoopThreadPool_;// 事件循环线程池
  bool started_;                                             // 服务器是否已启动
  std::shared_ptr<Channel> acceptChannel_;                   // 接受新连接的Channel
  int port_;                                                 // 监听的端口号
  int listenFd_;                                             // 监听socket的文件描述符
  static const int MAXFDS = 100000;                          // 最大文件描述符数量

  //数据库相关
  connection_pool *connPool = m_connPool; 
  string m_user;         //登陆数据库用户名
  string m_passWord;     //登陆数据库密码
  string m_databaseName; //使用数据库名
  int m_sql_num;
  int m_log_write;
  int m_close_log;
  HttpData *users;
};
