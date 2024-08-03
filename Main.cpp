#include <getopt.h> // 头文件，用于解析命令行参数
#include <string>
#include "EventLoop.h"    // 事件循环相关
#include "Server.h"       // 服务器相关
#include "base/Logging.h" // 日志相关
#include "base/MemoryPool.h"
#include "base/LFUCache.h"
#include "CGImysql/sql_connection_pool.h"

extern connection_pool *m_connPool;

connection_pool *m_connPool = nullptr; // 定义全局连接池变量

int main(int argc, char *argv[])
{
  int threadNum = 4;                       // 线程数量，默认值为4
  int port = 9190;                           // 服务器端口，默认值为9190
  std::string logPath = "./WebServer.log"; // 日志文件路径，默认值为当前目录下的WebServer.log

  //需要修改的数据库信息,登录名,密码,库名
  string user = "root";
  string passwd = "123";
  string databasename = "tws";

  // 解析命令行参数
  int opt;
  const char *str = "t:l:p:"; // 定义选项字符串，t表示线程数量，l表示日志路径，p表示端口号
  while ((opt = getopt(argc, argv, str)) != -1)
  { // getopt函数解析命令行参数
    switch (opt)
    {
    case 't':
    {
      threadNum = atoi(optarg); // 将线程数量参数转换为整数并赋值给threadNum
      break;
    }
    case 'l':
    {
      logPath = optarg; // 将日志路径参数赋值给logPath
      if (logPath.size() < 2 || optarg[0] != '/')
      {                                              // 检查日志路径是否有效
        printf("logPath should start with \"/\"\n"); // 如果无效，打印错误信息
        abort();                                     // 终止程序
      }
      break;
    }
    case 'p':
    {
      port = atoi(optarg); // 将端口号参数转换为整数并赋值给port
      break;
    }
    default:
      break;
    }
  }

  Logger::setLogFileName(logPath); // 设置日志文件路径

// STL库在多线程上应用
#ifndef _PTHREADS
  LOG << "_PTHREADS is not defined !"; // 如果未定义_PTHREADS，记录警告信息
#endif

  init_MemoryPool(); // 初始化内存池
  getCache();        // 初始化缓存。假设 getCache() 函数会初始化全局缓存实例

  // EventLoop *mainLoop = new EventLoop();
  EventLoop *mainLoop = newElement<EventLoop>();         // 创建主事件循环对象
  // EventLoop mainLoop;                              // 创建主事件循环对象
  Server myHTTPServer(mainLoop, threadNum, port, user, passwd, databasename); // 创建服务器对象，并传入事件循环对象、线程数量和端口号
  // 初始化数据库连接池
  m_connPool = connection_pool::GetInstance();
  m_connPool->init("localhost", user, passwd, databasename, 3306, 10, 0);
  //初始化数据库连接池
  myHTTPServer.sql_pool();                        // 数据库连接池初始化
  myHTTPServer.start();                           // 启动服务器
  mainLoop->loop();                               // 开始事件循环
  deleteElement(mainLoop);                        // 删除主事件循环对象
  return 0;                                       // 程序正常退出
}
