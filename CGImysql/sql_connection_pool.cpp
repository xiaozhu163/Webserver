#include <mysql/mysql.h>         // MySQL头文件
#include <stdio.h>               // 标准输入输出头文件
#include <string>                // C++字符串库
#include <string.h>              // C字符串处理库
#include <stdlib.h>              // 标准库头文件
#include <list>                  // C++列表容器
#include <pthread.h>             // 线程库
#include <iostream>              // 标准输入输出流
#include "sql_connection_pool.h" // 自定义的数据库连接池头文件
#include "../base/Logging.h"     // 自定义的日志库头文件

using namespace std;

// 构造函数，初始化当前连接数和空闲连接数
connection_pool::connection_pool()
{
    m_CurConn = 0;
    m_FreeConn = 0;
}

// 获取连接池实例，使用单例模式
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

// 构造初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
    LOG << "Entering connection_pool::init"; // 日志记录进入函数
    m_url = url;                             // 设置数据库URL
    m_Port = Port;                           // 设置数据库端口
    m_User = User;                           // 设置数据库用户名
    m_PassWord = PassWord;                   // 设置数据库密码
    m_DatabaseName = DBName;                 // 设置数据库名
    m_close_log = close_log;                 // 设置日志开关

    LOG << "Initializing connection pool with URL: " << url
        << ", User: " << User
        << ", DBName: " << DBName
        << ", Port: " << Port
        << ", MaxConn: " << MaxConn; // 日志记录初始化信息

    for (int i = 0; i < MaxConn; i++) // 循环创建最大连接数的数据库连接
    {
        LOG << "Creating MySQL connection " << i + 1 << " of " << MaxConn; // 日志记录创建连接
        MYSQL *con = NULL;                                                 // 定义一个MySQL连接指针
        MYSQL *ret = NULL;                                                 // 定义一个MySQL返回指针

        LOG << "Calling mysql_init"; // 日志记录调用mysql_init
        ret = mysql_init(con);       // 初始化MySQL连接
        if (ret == NULL)             // 如果初始化失败
        {
            LOG << "MySQL Error: mysql_init() returns NULL"; // 日志记录错误
            exit(1);                                         // 退出程序
        }
        else
        {
            con = ret; // 如果成功，设置连接指针
        }

        LOG << "Calling mysql_real_connect";                                                                       // 日志记录调用mysql_real_connect
        ret = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0); // 连接到数据库
        if (ret == NULL)                                                                                           // 如果连接失败
        {
            string err_info(mysql_error(con));                                                     // 获取错误信息
            err_info = "MySQL Error[errno=" + std::to_string(mysql_errno(con)) + "]: " + err_info; // 格式化错误信息
            LOG << err_info;                                                                       // 日志记录错误信息
            exit(1);                                                                               // 退出程序
        }
        else
        {
            con = ret; // 如果成功，设置连接指针
        }

        LOG << "Adding connection to pool"; // 日志记录添加连接到连接池
        connList.push_back(con);            // 将连接添加到连接列表
        ++m_FreeConn;                       // 增加空闲连接数
    }

    LOG << "Initializing semaphore with m_FreeConn = " << m_FreeConn; // 日志记录初始化信号量
    reserve = sem(m_FreeConn);                                        // 初始化信号量
    m_MaxConn = m_FreeConn;                                           // 设置最大连接数
    LOG << "Connection pool initialized successfully";                // 日志记录初始化成功
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = NULL;

    if (0 == connList.size()) // 如果连接列表为空
        return NULL;

    reserve.wait(); // 等待信号量

    lock.lock(); // 加锁

    con = connList.front(); // 获取列表头部的连接
    connList.pop_front();   // 移除头部连接

    --m_FreeConn; // 减少空闲连接数
    ++m_CurConn;  // 增加当前使用的连接数

    lock.unlock(); // 解锁
    return con;    // 返回获取的连接
}

// 释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if (NULL == con) // 如果连接为空
        return false;

    lock.lock(); // 加锁

    connList.push_back(con); // 将连接放回列表
    ++m_FreeConn;            // 增加空闲连接数
    --m_CurConn;             // 减少当前使用的连接数

    lock.unlock(); // 解锁

    reserve.post(); // 增加信号量
    return true;
}

// 销毁数据库连接池
void connection_pool::DestroyPool()
{
    lock.lock();             // 加锁
    if (connList.size() > 0) // 如果连接列表不为空
    {
        list<MYSQL *>::iterator it;                             // 定义列表迭代器
        for (it = connList.begin(); it != connList.end(); ++it) // 遍历列表
        {
            MYSQL *con = *it; // 获取连接
            mysql_close(con); // 关闭连接
        }
        m_CurConn = 0;    // 重置当前使用的连接数
        m_FreeConn = 0;   // 重置空闲连接数
        connList.clear(); // 清空连接列表
    }
    lock.unlock(); // 解锁
}

// 当前空闲的连接数
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}

// 析构函数，销毁连接池
connection_pool::~connection_pool()
{
    DestroyPool();
}

// 连接资源管理类构造函数
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)
{
    *SQL = connPool->GetConnection(); // 获取一个连接

    conRAII = *SQL;      // 设置连接资源
    poolRAII = connPool; // 设置连接池资源
}

// 连接资源管理类析构函数
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII); // 释放连接
}
