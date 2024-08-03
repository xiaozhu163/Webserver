#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT; // 全局静态变量，用于保证只初始化一次的控制变量
static AsyncLogging *AsyncLogger_;                       // 全局静态变量，用于存储异步日志记录器指针
std::string Logger::logFileName_ = "./WebServer.log";    // 日志文件名的静态成员变量

void once_init() // 初始化异步日志记录器的函数
{
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName()); // 创建一个新的异步日志记录器实例，并传递日志文件名
    AsyncLogger_->start(); // 启动异步日志记录器
}

void output(const char *msg, int len) // 输出日志消息的函数
{

    pthread_once(&once_control_, once_init); // 确保once_init函数只执行一次 这个函数调用确保 once_init 函数只执行一次。如果多个线程调用 pthread_once，只有第一个调用会执行 once_init，后续的调用将不会再次执行 once_init。
    AsyncLogger_->append(msg, len); // 将日志消息追加到异步日志记录器中
}

Logger::Impl::Impl(const char *fileName, int line) // Logger::Impl 构造函数，初始化日志记录实现类
    : stream_(),                                   // 初始化日志流
      line_(line),                                 // 初始化行号
      basename_(fileName)                          // 初始化文件名
{
    formatTime(); // 格式化当前时间
}

void Logger::Impl::formatTime() // 格式化当前时间并将其写入日志流
{
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday(&tv, NULL);                            // 获取当前时间
    time = tv.tv_sec;                                   // 获取秒级时间戳
    struct tm *p_time = localtime(&time);               // 将时间戳转换为本地时间
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time); // 格式化时间字符串
    stream_ << str_t;                                   // 将时间字符串写入日志流
}

Logger::Logger(const char *fileName, int line) // Logger 构造函数，初始化日志记录器
    : impl_(fileName, line)                    // 初始化实现类
{
}

Logger::~Logger() // Logger 析构函数，负责输出日志信息 
{

    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n'; // 将文件名和行号信息追加到日志流中
    const LogStream::Buffer &buf(stream().buffer()); // 获取日志流中的数据缓冲区
    output(buf.data(), buf.length()); // 输出日志流中的数据 第一次写日志时激活日志线程
}
