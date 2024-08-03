#pragma once

#include <pthread.h> // 包含POSIX线程库
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h" // 包含LogStream类的头文件
#include "../CGImysql/sql_connection_pool.h"

class connection_pool; // 前向声明connection_pool类
extern connection_pool* m_connPool;

class AsyncLogging; // 前向声明AsyncLogging类

class Logger
{
public:
    Logger(const char *fileName, int line); // 构造函数，接受文件名和行号
    ~Logger(); // 析构函数

    LogStream &stream() { return impl_.stream_; } // 返回LogStream对象的引用
    static void setLogFileName(std::string fileName) { logFileName_ = fileName; } // 静态方法设置日志文件名
    static std::string getLogFileName() { return logFileName_; } // 静态方法获取日志文件名

private:
    class Impl // 嵌套的Impl类，实际实现Logger的功能
    {
    public:
        Impl(const char *fileName, int line); // Impl类的构造函数，接受文件名和行号

        void formatTime(); // 格式化当前时间
        LogStream stream_;     // LogStream对象，用于处理日志流
        int line_;             // 日志的行号
        std::string basename_; // 文件的基本名（不带路径）
    };

    Impl impl_;                      // Impl对象，实际处理日志功能
    static std::string logFileName_; // 静态成员，存储日志文件名
};

// 宏定义，简化日志的记录方式，自动记录文件名和调用时当前行号
#define LOG Logger(__FILE__, __LINE__).stream()

//作用域限制：LOG 宏展开后生成的 Logger 对象只在当前表达式语句的作用域内有效。也就是说，它的生命周期从 LOG 宏出现的地方开始，直到整个表达式语句结束为止。

