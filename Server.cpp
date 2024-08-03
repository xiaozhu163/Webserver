#include "Server.h"
#include <arpa/inet.h>    // 用于互联网地址操作
#include <netinet/in.h>   // 包含IPv4地址结构体定义
#include <sys/socket.h>   // 包含套接字函数和数据结构定义
#include <functional>     // 用于std::bind
#include "Util.h"         // 包含实用函数的声明
#include "base/Logging.h" // 包含日志功能的声明
#include "base/MemoryPool.h"
#include "base/LFUCache.h"

extern connection_pool* m_connPool;

// Server构造函数，初始化成员变量
Server::Server(EventLoop *loop, int threadNum, int port, string user, string password, string m_databaseName)
    : loop_(loop),           // 主事件循环， eventloop中会自动创建epoll对象poller，然后poller中会创建定时器管理器，在poller中每次add事件 mod事件，或者事件发生处理事件时，
                             // 主loop不设置定时器；将time设置为0，进行判断时就都不会创建定时器，time设置为0的不会添加新的定时器到Epoll的poller的定时器管理器
                             // 普通的channel会对应一个httpdata，因为普通的连接对应其余子线程对应的loop，在主线程接收他们连接分为子loop时会创建一个http_data，在http_data中创建channel；
      threadNum_(threadNum), // 线程数
      eventLoopThreadPool_(newElement<EventLoopThreadPool>(loop, threadNum), DeleteElement<EventLoopThreadPool>()), // 事件循环线程池
      // eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)), // 事件循环线程池
      started_(false),                    // 服务器是否启动
      acceptChannel_(new Channel(loop_)), // 监听通道
      port_(port),                        // 端口号
      listenFd_(socket_bind_listen(port_)),
      m_user(user),
      m_passWord(password),
      m_databaseName(m_databaseName)
{                                   // 绑定并监听端口
  acceptChannel_->setFd(listenFd_); // 设置监听通道的文件描述符
  handle_for_sigpipe();             // 处理SIGPIPE信号
  if (setSocketNonBlocking(listenFd_) < 0)
  {
    perror("set socket non block failed"); // 设置监听套接字为非阻塞模式
    abort();
  }
}

// 启动服务器
void Server::start()
{
  eventLoopThreadPool_->start(); // 启动事件循环线程池
  // acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);                      // 设置监听事件为EPOLLIN和EPOLLET
  acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));  // 设置读事件处理器
  acceptChannel_->setConnHandler(bind(&Server::handThisConn, this)); // 设置连接处理器，每次处理完连接事件之后就执行一下这个
  loop_->addToPoller(acceptChannel_, 0);                             // 将监听通道添加到事件循环中 这里设置time等于0，相当于主loop不设置定时器，因为后边会判断，只有这里的大于0才会设置或者增加定时器
  started_ = true;                                                   // 标记服务器已启动
}

// 处理新连接
void Server::handNewConn()
{
  struct sockaddr_in client_addr;                      // 客户端地址结构体
  memset(&client_addr, 0, sizeof(struct sockaddr_in)); // 清空客户端地址
  socklen_t client_addr_len = sizeof(client_addr);     // 客户端地址长度
  int accept_fd = 0;
  // 接受新连接
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0)
  {
    EventLoop *loop = eventLoopThreadPool_->getNextLoop(); // 获取下一个事件循环
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
        << ntohs(client_addr.sin_port);
    // cout << "new connection" << endl;
    // cout << inet_ntoa(client_addr.sin_addr) << endl;
    // cout << ntohs(client_addr.sin_port) << endl;
    /*
    // TCP的保活机制默认是关闭的
    int optval = 0;
    socklen_t len_optval = 4;
    getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
    cout << "optval ==" << optval << endl;
    */

    if (accept_fd >= MAXFDS) // 限制服务器的最大并发连接数
    {
      close(accept_fd);
      continue;
    }

    if (setSocketNonBlocking(accept_fd) < 0) // 设为非阻塞模式
    {
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    setSocketNodelay(accept_fd); // 设置TCP_NODELAY选项
    // setSocketNoLinger(accept_fd);
    std::shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd)); // 创建新的HttpData对象 创建时就绑定了其对应的loop和文件描述符
    req_info->getChannel()->setHolder(req_info);                       // 设置通道持有者--http_data，将channel和http_data绑定
    loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));       // 将新事件添加到事件循环中  这个队列的作用就是将所有的新连接的事件添加到子线程的loop中，每个loop都有一个这样的队列，每次绑定函数执行时都会将新连接的事件添加到对应的loop中
  }
  acceptChannel_->setEvents(EPOLLIN | EPOLLET); // 重置监听通道的事件
}

void Server::sql_pool()
{
    //初始化数据库连接池
    // connPool = connection_pool::GetInstance();
    // connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}

