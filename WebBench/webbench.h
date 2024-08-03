// webbench.h
#pragma  once
#ifndef WEB_BENCH_H_ // 如果没有定义 WEB_BENCH_H_，则进行定义。这是为了防止重复包含该头文件。
#define WEB_BENCH_H_

#include <stdio.h>      // 标准输入输出库
#include <stdlib.h>     // 标准库，包括内存分配、控制进程等
#include <stdarg.h>     // 可变参数函数库
#include <string.h>     // 字符串操作库
#include <signal.h>     // 信号处理库
#include <time.h>       // 时间库
#include <unistd.h>     // UNIX 标准函数定义
#include <fcntl.h>      // 文件控制定义
#include <netinet/in.h> // Internet地址族
#include <arpa/inet.h>  // Internet操作函数
#include <netdb.h>      // 域名解析库
#include <getopt.h>     // 处理命令行参数
// #include <rpc/types.h>  // RPC类型定义
#include <sys/types.h>  // 基本系统数据类型
#include <sys/socket.h> // 套接字库
#include <sys/time.h>   // 时间操作库
#include <iostream>     // C++标准输入输出流库

// 使用标准命名空间中的cout和endl
using std::cout;
using std::endl;

// 声明一个函数，用于解析命令行参数。
// 参数：
//   argc - 参数的数量。
//   argv - 参数的数组。
void ParseArg(int argc, char *argv[]);

// 声明一个函数，用于构建HTTP请求。
// 参数：
//   url - 要请求的URL地址。
void BuildRequest(const char *url);

// 声明一个函数，用于启动WebBench测试。
void WebBench();

#endif // WEB_BENCH_H_  // 结束WEB_BENCH_H_的条件编译。
