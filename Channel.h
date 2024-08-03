#pragma once

#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

// 前向声明
class EventLoop;
class HttpData;

class Channel
{
private:
    typedef std::function<void()> CallBack;
    EventLoop *loop_;       // 指向所属的事件循环
    int fd_;                // 该Channel关联的文件描述符
    __uint32_t events_;     // 感兴趣的事件 Channel正在监听的事件
    __uint32_t revents_;    // 已经发生的事件 或者说 返回的就绪事件
    __uint32_t lastEvents_; // 上一次的事件 （主要⽤于记录如果本次事件和上次事件⼀样 就没必要调⽤epoll_mod）

    // 方便找到上层持有该Channel的对象 一个channel对应一个httpdata 一个 Channel 对应一个 HttpData 对象。Channel 负责处理某个文件描述符（通常是一个网络套接字）的事件，而 HttpData 则负责处理与该套接字相关的 HTTP 请求和响应数据。
    std::weak_ptr<HttpData> holder_;

private:
    // 以下为HTTP请求解析相关的私有成员函数
    // 解析URI（统一资源标识符）
    int parse_URI();
    // 解析HTTP请求头部
    int parse_Headers();
    // 分析HTTP请求
    int analysisRequest();

    // 事件处理的回调函数
    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;

public:
    // 构造函数和析构函数
    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // 获取和设置文件描述符
    int getFd();
    void setFd(int fd);

    // 设置和获取持有该Channel的HttpData对象
    void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
    
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    // 设置事件处理的回调函数
    void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
    void setWriteHandler(CallBack &&writeHandler) { writeHandler_ = writeHandler; }
    void setErrorHandler(CallBack &&errorHandler) { errorHandler_ = errorHandler; }
    void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

    // 处理事件
    void handleEvents()
    {
        events_ = 0; // 将感兴趣的事件置为0，以便在处理完当前事件后重新设置

        // 如果发生 EPOLLHUP 事件且不是 EPOLLIN 事件，则关闭连接
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0; // 关闭连接，将感兴趣的事件置为0
            return;      // 返回，不再处理其他事件
        }

        // 如果发生 EPOLLERR 错误事件，则执行错误处理函数
        if (revents_ & EPOLLERR)
        {
            if (errorHandler_)   // 如果错误处理函数存在
                errorHandler_(); // 调用错误处理函数
            events_ = 0;         // 将感兴趣的事件置为0，表示处理完毕
            return;              // 返回，不再处理其他事件
        }

        // 如果发生 EPOLLIN、EPOLLPRI 或 EPOLLRDHUP 事件，则处理读事件
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            handleRead(); // 处理读事件
        }

        // 如果发生 EPOLLOUT 事件，则处理写事件
        if (revents_ & EPOLLOUT)
        {
            handleWrite(); // 处理写事件
        }

        // 处理连接相关事件
        handleConn();
    }

    // 处理读、写、错误和连接事件的函数
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    // 设置和获取事件
    void setRevents(__uint32_t ev) { revents_ = ev; }
    void setEvents(__uint32_t ev) { events_ = ev; }
    __uint32_t &getEvents() { return events_; }

    // 比较当前事件和上一次的事件是否相同，并更新上一次的事件
    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    // 获取上一次的事件
    __uint32_t getLastEvents() { return lastEvents_; }
};

// 定义智能指针类型
typedef std::shared_ptr<Channel> SP_Channel;
