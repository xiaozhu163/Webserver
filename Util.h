#pragma once
#include <cstdlib>
#include <string>

ssize_t readn(int fd, void *buff, size_t n); // 从文件描述符fd中读取n字节的数据到缓冲区buff中

ssize_t readn(int fd, std::string &inBuffer, bool &zero); // 从文件描述符fd中读取数据并追加到字符串inBuffer中
                                                          // zero用于标记是否读取到0字节

ssize_t readn(int fd, std::string &inBuffer); // 从文件描述符fd中读取数据并追加到字符串inBuffer中

ssize_t writen(int fd, void *buff, size_t n); // 向文件描述符fd中写入n字节的数据从缓冲区buff中

ssize_t writen(int fd, std::string &sbuff); // 将字符串sbuff中的数据写入文件描述符fd中

void handle_for_sigpipe(); // 处理SIGPIPE信号

int setSocketNonBlocking(int fd); // 设置文件描述符fd为非阻塞模式

void setSocketNodelay(int fd); // 设置文件描述符fd为无延迟模式

void setSocketNoLinger(int fd); // 设置文件描述符fd关闭时不执行linger操作

void shutDownWR(int fd); // 关闭文件描述符fd的写端

int socket_bind_listen(int port); // 绑定并监听指定端口，返回监听套接字
