#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cctype>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"
#include "base/MemoryPool.h"
#include "base/LFUCache.h"
#include "lock/locker.h"
#include "CGImysql/sql_connection_pool.h"
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
// #include "./log/log.h"

class EventLoop;
class TimerNode;
class Channel;

extern connection_pool *m_connPool; // 只声明，不定义

using namespace std;

// 定义处理状态枚举
enum ProcessState {
  STATE_PARSE_URI = 1,    // 解析URI
  STATE_PARSE_HEADERS,    // 解析头部
  STATE_RECV_BODY,        // 接收请求体
  STATE_ANALYSIS,         // 解析请求
  STATE_FINISH            // 完成处理
};

// 定义URI解析状态枚举
enum URIState {
  PARSE_URI_AGAIN = 1,    // 重新解析URI
  PARSE_URI_ERROR,        // 解析URI出错
  PARSE_URI_SUCCESS,      // 解析URI成功
};

// 定义头部解析状态枚举
enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,  // 解析头部成功
  PARSE_HEADER_AGAIN,        // 重新解析头部
  PARSE_HEADER_ERROR         // 解析头部出错
};

// 定义请求解析状态枚举
enum AnalysisState {
  ANALYSIS_SUCCESS = 1,   // 解析成功
  ANALYSIS_ERROR          // 解析出错
};

// 定义头部解析过程状态枚举
enum ParseState {
  H_START = 0,            // 开始解析
  H_KEY,                  // 解析键
  H_COLON,                // 解析冒号
  H_SPACES_AFTER_COLON,   // 冒号后的空格
  H_VALUE,                // 解析值
  H_CR,                   // 回车
  H_LF,                   // 换行
  H_END_CR,               // 结束的回车
  H_END_LF                // 结束的换行
};

// 定义连接状态枚举
enum ConnectionState {
  H_CONNECTED = 0,        // 已连接
  H_DISCONNECTING,        // 断开中
  H_DISCONNECTED          // 已断开
};

// 定义HTTP方法枚举
enum HttpMethod {
  METHOD_POST = 1,        // POST方法
  METHOD_GET,             // GET方法
  METHOD_HEAD             // HEAD方法
};

// 定义HTTP版本枚举
enum HttpVersion {
  HTTP_10 = 1,            // HTTP 1.0
  HTTP_11                 // HTTP 1.1
};

// #define FILENAME_LEN 200    // 文件名长度

// MimeType类用于获取MIME类型
class MimeType {
private:
    // 初始化MIME类型映射
    static void init();
    
    // 存储文件后缀与MIME类型的映射
    static std::unordered_map<std::string, std::string> mime;
    
    // 构造函数设为私有，防止创建对象
    MimeType();
    
    // 拷贝构造函数设为私有，防止复制
    MimeType(const MimeType &m);

public:
    // 根据文件后缀获取MIME类型
    static std::string getMime(const std::string &suffix);

private:
    // 控制初始化的pthread_once_t变量
    static pthread_once_t once_control;
};

// HttpData类，负责处理HTTP请求和响应
class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    // 构造函数，初始化HTTP数据
    HttpData(EventLoop *loop, int connfd);
    
    // 析构函数，关闭文件描述符
    ~HttpData() { close(fd_); }
    
    // 重置HTTP数据
    void reset();
    
    // 分离定时器
    void seperateTimer();
    
    // 关联定时器
    void linkTimer(std::shared_ptr<TimerNode> mtimer) {
        // shared_ptr重载了bool, 但weak_ptr没有
        timer_ = mtimer;
    }
    
    // 获取关联的Channel
    std::shared_ptr<Channel> getChannel() { return channel_; }
    
    // 获取事件循环
    EventLoop *getLoop() { return loop_; }
    
    // 处理关闭事件
    void handleClose();
    
    // 处理新的事件
    void newEvent();

    // 数据库相关
    void initmysql_result(connection_pool *connPool);
    bool authenticateUser(const string& username, const string& password);
    // 声明 urlDecode 函数
    static std::string urlDecode(const std::string& str);

private:
    EventLoop *loop_;                    // 事件循环
    std::shared_ptr<Channel> channel_;   // 关联的Channel
    int fd_;                             // 文件描述符
    std::string inBuffer_;               // 输入缓冲区
    std::string outBuffer_;              // 输出缓冲区
    bool error_;                         // 是否有错误
    ConnectionState connectionState_;    // 连接状态

    HttpMethod method_;                  // HTTP方法
    HttpVersion HTTPVersion_;            // HTTP版本
    std::string fileName_;               // 请求的文件名
    std::string path_;                   // 请求路径
    int nowReadPos_;                     // 当前读取位置
    ProcessState state_;                 // 处理状态
    ParseState hState_;                  // 头部解析状态
    bool keepAlive_;                     // 是否保持连接
    std::map<std::string, std::string> headers_;  // 请求头部
    std::weak_ptr<TimerNode> timer_;     // 关联的定时器
    // connection_pool* connPool_;  // 数据库连接池

    // 数据库查询缓存
    // std::unordered_map<std::string, std::string> users;
    std::mutex m_lock;
    std::string m_url;
    std::string m_real_file;
    std::string m_string;
    struct stat m_file_stat;
    char* m_file_address;

    int cgi;        //是否启用的POST
    int bytes_have_send;
    char *doc_root;

    // 处理读事件
    void handleRead();
    
    // 处理写事件
    void handleWrite();
    
    // 处理连接事件
    void handleConn();
    
    // 处理错误事件
    void handleError(int fd, int err_num, std::string short_msg);
    
    // 解析URI
    URIState parseURI();
    
    // 解析头部
    HeaderState parseHeaders();
    
    // 解析请求
    AnalysisState analysisRequest();

    bool parseRequest();
    void generateResponse();

};
