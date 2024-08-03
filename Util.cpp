#include "Util.h"

#include <errno.h>       // 错误码定义
#include <fcntl.h>       // 文件控制定义
#include <netinet/in.h>  // 网络地址定义
#include <netinet/tcp.h> // TCP协议定义
#include <signal.h>      // 信号处理定义
#include <string.h>      // 字符串处理定义
#include <sys/socket.h>  // 套接字定义
#include <unistd.h>      // Unix标准定义

// 定义一个最大缓冲区大小
const int MAX_BUFF = 4096;

// 从文件描述符fd中读取n字节的数据到缓冲区buff中
ssize_t readn(int fd, void *buff, size_t n) {
  size_t nleft = n;     // 剩余要读取的字节数
  ssize_t nread = 0;    // 每次实际读取的字节数
  ssize_t readSum = 0;  // 总共读取的字节数
  char *ptr = (char *)buff;  // 缓冲区指针

  // 循环读取直到读取完所有字节或出错
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)  // 如果被信号中断，则重新读取
        nread = 0;
      else if (errno == EAGAIN) {
        return readSum;  // 如果资源暂不可用，则返回已读取的字节数
      } else {
        return -1;  // 其他错误，返回-1
      }
    } else if (nread == 0)  // 读到文件末尾，退出循环
      break;
    readSum += nread;  // 更新总共读取的字节数
    nleft -= nread;    // 更新剩余要读取的字节数
    ptr += nread;      // 更新缓冲区指针
  }
  return readSum;  // 返回总共读取的字节数
}

// 从文件描述符fd中读取数据并追加到字符串inBuffer中
// zero用于标记是否读取到0字节（连接已关闭）
ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
    char buffer[4096]; // 临时缓冲区
    zero = false; // 初始化 zero 标记
    ssize_t totalBytesRead = 0;
    
    while (true) {
        ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            // 发生错误
            return -1;
        } else if (bytesRead == 0) {
            // 连接关闭
            zero = true;
            return totalBytesRead;
        }
        
        totalBytesRead += bytesRead;
        inBuffer.append(buffer, bytesRead);
        
        // 如果读取到的数据长度与请求体长度相等，则结束读取
        if (inBuffer.size() >= static_cast<size_t>(totalBytesRead)) {
            break;
        }
    }
    
    return totalBytesRead;
}

// 从文件描述符fd中读取数据并追加到字符串inBuffer中
ssize_t readn(int fd, std::string &inBuffer) {
  ssize_t nread = 0;    // 每次实际读取的字节数
  ssize_t readSum = 0;  // 总共读取的字节数

  // 循环读取直到读取完所有数据或出错
  while (true) {
    char buff[MAX_BUFF];  // 临时缓冲区
    if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
      if (errno == EINTR)  // 如果被信号中断，则重新读取
        continue;
      else if (errno == EAGAIN) {
        return readSum;  // 如果资源暂不可用，则返回已读取的字节数
      } else {
        perror("read error");  // 其他错误，输出错误信息并返回-1
        return -1;
      }
    } else if (nread == 0) {  // 读到文件末尾
      break;
    }
    readSum += nread;  // 更新总共读取的字节数
    inBuffer += std::string(buff, buff + nread);  // 追加读取的数据到inBuffer中
  }
  return readSum;  // 返回总共读取的字节数
}

// 向文件描述符fd中写入n字节的数据从缓冲区buff中
ssize_t writen(int fd, void *buff, size_t n) {
  size_t nleft = n;     // 剩余要写入的字节数
  ssize_t nwritten = 0; // 每次实际写入的字节数
  ssize_t writeSum = 0; // 总共写入的字节数
  char *ptr = (char *)buff; // 缓冲区指针

  // 循环写入直到写入完所有字节或出错
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) { // 如果被信号中断，则重新写入
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN) {
          return writeSum; // 如果资源暂不可用，则返回已写入的字节数
        } else
          return -1; // 其他错误，返回-1
      }
    }
    writeSum += nwritten; // 更新总共写入的字节数
    nleft -= nwritten;    // 更新剩余要写入的字节数
    ptr += nwritten;      // 更新缓冲区指针
  }
  return writeSum; // 返回总共写入的字节数
}

// 将字符串sbuff中的数据写入文件描述符fd中
ssize_t writen(int fd, std::string &sbuff) {
  size_t nleft = sbuff.size(); // 剩余要写入的字节数
  ssize_t nwritten = 0;        // 每次实际写入的字节数
  ssize_t writeSum = 0;        // 总共写入的字节数
  const char *ptr = sbuff.c_str(); // 缓冲区指针

  // 循环写入直到写入完所有字节或出错
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) { // 如果被信号中断，则重新写入
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN)
          break; // 如果资源暂不可用，则退出循环
        else
          return -1; // 其他错误，返回-1
      }
    }
    writeSum += nwritten; // 更新总共写入的字节数
    nleft -= nwritten;    // 更新剩余要写入的字节数
    ptr += nwritten;      // 更新缓冲区指针
  }
  if (writeSum == static_cast<int>(sbuff.size()))
    sbuff.clear(); // 如果所有数据已写入，清空缓冲区
  else
    sbuff = sbuff.substr(writeSum); // 否则更新缓冲区
  return writeSum; // 返回总共写入的字节数
}

// 处理SIGPIPE信号，以避免写入到已关闭的套接字时进程崩溃
void handle_for_sigpipe() {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN; // 忽略SIGPIPE信号
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL)) return;
}

// 设置文件描述符fd为非阻塞模式
int setSocketNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
}

// 设置文件描述符fd为无延迟模式（禁用Nagle算法）
void setSocketNodelay(int fd) {
  int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

// 设置文件描述符fd关闭时不执行linger操作
void setSocketNoLinger(int fd) {
  struct linger linger_;
  linger_.l_onoff = 1;
  linger_.l_linger = 30;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_, sizeof(linger_));
}

// 关闭文件描述符fd的写端
void shutDownWR(int fd) {
  shutdown(fd, SHUT_WR);
  // printf("shutdown\n");
}

// 绑定并监听指定端口，返回监听套接字
int socket_bind_listen(int port) {
  // 检查port值，确保在正确区间范围内
  if (port < 0 || port > 65535) return -1;

  // 创建socket(IPv4 + TCP)，返回监听描述符
  int listen_fd = 0;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

  // 消除bind时"Address already in use"错误
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
    close(listen_fd);
    return -1;
  }

  // 设置服务器IP和Port，并与监听描述符绑定
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    close(listen_fd);
    return -1;
  }

  // 开始监听，最大等待队列长度为2048
  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  // 无效监听描述符，返回错误
  if (listen_fd == -1) {
    close(listen_fd);
    return -1;
  }
  return listen_fd; // 返回监听描述符
}
