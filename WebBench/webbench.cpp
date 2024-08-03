#include "webbench.h"

// 定义HTTP请求方法的宏
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3

// 定义Webbench工具的版本
#define WEBBENCH_VERSION "v1.5"

// 定义请求的大小
#define REQUEST_SIZE 2048
#define MAXHOSTNAMELEN 256

// 初始化全局变量
static int force = 0;                   // 默认需要等待服务器响应
static int force_reload = 0;            // 默认不重新发送请求
static int clients_num = 1;             // 默认客户端数量
static int request_time = 30;           // 默认模拟请求时间
static int http_version = 2;            // 默认HTTP协议版本 0:http/0.9, 1:http/1.0, 2:http/1.1
static char *proxy_host = NULL;         // 默认无代理服务器
static int port = 9190;                 // 默认访问9190端口
static int is_keep_alive = 0;           // 默认不支持keep alive
static int request_method = METHOD_GET; // 默认请求方法
static int pipeline[2];                 // 用于父子进程通信的管道
static char host[MAXHOSTNAMELEN];       // 存放目标服务器的网络地址
static char request_buf[REQUEST_SIZE];  // 存放请求报文的字节流
static int success_count = 0;           // 请求成功的次数
static int failed_count = 0;            // 失败的次数
static int total_bytes = 0;             // 服务器响应总字节数
volatile bool is_expired = false;       // 子进程访问服务器是否超时

// 显示使用方法
static void Usage()
{
    fprintf(stderr,
            "Usage: webbench [option]... URL\n"
            " -f|--force 不等待服务器响应\n"
            " -r|--reload 重新请求加载(无缓存)\n"
            " -9|--http09 使用http0.9协议来构造请求\n"
            " -1|--http10 使用http1.0协议来构造请求\n"
            " -2|--http11 使用http1.1协议来构造请求\n"
            " -k|--keep_alive 客户端是否支持keep alive\n"
            " -t|--time <sec> 运行时间，单位：秒，默认为30秒\n"
            " -c|--clients_num <n> 创建多少个客户端，默认为1个\n"
            " -p|--proxy <server:port> 使用代理服务器发送请求\n"
            " --get 使用GET请求方法\n"
            " --head 使用HEAD请求方法\n"
            " --options 使用OPTIONS请求方法\n"
            " --trace 使用TRACE请求方法\n"
            " -?|-h|--help 显示帮助信息\n"
            " -V|--version 显示版本信息\n");
}

// 构造长选项与短选项的对应关系
static const struct option OPTIONS[] = {
    {"force", no_argument, &force, 1},                         // -f|--force 不等待服务器响应
    {"reload", no_argument, &force_reload, 1},                 // -r|--reload 重新请求加载(无缓存)
    {"http09", no_argument, NULL, '9'},                        // -9|--http09 使用http0.9协议来构造请求
    {"http10", no_argument, NULL, '1'},                        // -1|--http10 使用http1.0协议来构造请求
    {"http11", no_argument, NULL, '2'},                        // -2|--http11 使用http1.1协议来构造请求
    {"keep_alive", no_argument, &is_keep_alive, 1},            // -k|--keep_alive 客户端是否支持keep alive
    {"time", required_argument, NULL, 't'},                    // -t|--time <sec> 运行时间，单位：秒
    {"clients", required_argument, NULL, 'c'},                 // -c|--clients_num <n> 创建多少个客户端
    {"proxy", required_argument, NULL, 'p'},                   // -p|--proxy <server:port> 使用代理服务器发送请求
    {"get", no_argument, &request_method, METHOD_GET},         // --get 使用GET请求方法
    {"head", no_argument, &request_method, METHOD_HEAD},       // --head 使用HEAD请求方法
    {"options", no_argument, &request_method, METHOD_OPTIONS}, // --options 使用OPTIONS请求方法
    {"trace", no_argument, &request_method, METHOD_TRACE},     // --trace 使用TRACE请求方法
    {"help", no_argument, NULL, '?'},                          // -?|-h|--help 显示帮助信息
    {"version", no_argument, NULL, 'V'},                       // -V|--version 显示版本信息
    {NULL, 0, NULL, 0}};                                       // 结束标记

// 打印消息
static void PrintMessage(const char *url)
{
    // 打印开始压测的标志
    printf("===================================开始压测===================================\n");

    // 根据请求方法打印相应的消息
    switch (request_method)
    {
    case METHOD_GET:
        printf("GET");
        break;
    case METHOD_OPTIONS:
        printf("OPTIONS");
        break;
    case METHOD_HEAD:
        printf("HEAD");
        break;
    case METHOD_TRACE:
        printf("TRACE");
        break;
    default:
        printf("GET");
        break;
    }

    // 打印URL
    printf(" %s", url);

    // 根据HTTP版本打印相应的消息
    switch (http_version)
    {
    case 0:
        printf(" (Using HTTP/0.9)");
        break;
    case 1:
        printf(" (Using HTTP/1.0)");
        break;
    case 2:
        printf(" (Using HTTP/1.1)");
        break;
    }

    // 打印客户端数量和每个进程的运行时间
    printf("\n客户端数量 %d, 每个进程运行 %d秒", clients_num, request_time);

    // 如果选择提前关闭连接，打印相应的消息
    if (force)
    {
        printf(", 选择提前关闭连接");
    }

    // 如果使用代理服务器，打印代理服务器信息
    if (proxy_host != NULL)
    {
        printf(", 经由代理服务器 %s:%d", proxy_host, port);
    }

    // 如果选择无缓存，打印相应的消息
    if (force_reload)
    {
        printf(", 选择无缓存");
    }

    // 打印结束换行符
    printf("\n");
}

/*
⾸先定义了⼀些记录状态的变量，定义了打印使⽤说明和结果的函数。然后是两个核⼼函数ConnectServer(2)、Worker(3)，其中
    * ConnectServer(2) 的作⽤是与 URL 所在的服务端建⽴ TCP 连接，
    * Worker(3) 则是在⼦进程中创建客户端与服务器发送信息。
*/

// ConnectServer(2) 的作⽤是与 URL 所在的服务端建⽴ TCP 连接
static int ConnectServer(const char *server_host, int server_port)
{
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字，协议类型: IPv4，网络数据类型: 字节流，网络协议: TCP协议
    if (client_sockfd < 0)                               // 检查套接字创建是否成功
    {
        return client_sockfd; // 如果失败，返回错误码
    }

    struct sockaddr_in server_addr;               // 定义服务器地址结构
    struct hostent *host_name;                    // 定义主机信息结构
    memset(&server_addr, 0, sizeof(server_addr)); // 初始化服务器地址结构为0
    server_addr.sin_family = AF_INET;             // 设置地址族为IPv4

    unsigned long ip = inet_addr(server_host); // 将服务器的主机名或IP地址转换为网络字节序IP地址
    if (ip != INADDR_NONE)
    {

        memcpy(&server_addr.sin_addr, &ip, sizeof(ip)); // 如果是有效的IP地址，直接复制到服务器地址结构
    }
    else
    {
        host_name = gethostbyname(server_host); // 否则，通过主机名获取IP地址
        if (host_name == NULL)
        {
            return -1; // 获取主机信息失败，返回-1
        }
        memcpy(&server_addr.sin_addr, host_name->h_addr, host_name->h_length); // 将主机信息结构中的地址复制到服务器地址结构
    }

    server_addr.sin_port = htons(server_port);                                            // 设置服务器端口号，转换为网络字节序
    if (connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) // 发起TCP连接（三次握手建立连接）
    {
        close(client_sockfd); // 连接失败，关闭套接字以避免套接字泄漏
        return -1;            // 返回-1表示连接失败
    }
    return client_sockfd; // 返回套接字描述符，表示连接成功
}

// 闹钟信号处理函数
static void AlarmHandler(int signal)
{
    is_expired = true; // 设置全局变量is_expired为true表示超时
}

// 子进程创建客户端去请求服务器
static void Worker(const char *server_host, const int server_port, const char *request)
{
    int client_sockfd = -1;             // 初始化客户端套接字
    int request_size = strlen(request); // 请求报文的大小
    const int response_size = 1500;     // 响应缓冲区的大小
    char response_buf[response_size];   // 响应缓冲区

    // 设置闹钟信号处理函数
    struct sigaction signal_action;
    signal_action.sa_handler = AlarmHandler; // 指定信号处理函数
    signal_action.sa_flags = 0;              // 无特殊标志
    if (sigaction(SIGALRM, &signal_action, NULL))
    {
        exit(1); // 设置信号处理函数失败，退出程序
    }
    alarm(request_time); // 设置闹钟，定时触发SIGALRM信号

    if (is_keep_alive)
    {
        // 一直到成功建立连接才退出while循环
        while (client_sockfd == -1)
        {
            client_sockfd = ConnectServer(server_host, server_port); // 尝试连接服务器
        }
        // cout << "1. 建立连接（TCP三次握手）成功!" << endl;

    keep_alive:
        while (true)
        {
            if (is_expired)
            {
                // 收到闹钟信号，子进程工作结束
                if (failed_count > 0)
                {
                    failed_count--;
                }
                return;
            }
            if (client_sockfd < 0)
            {
                failed_count++;
                continue;
            }
            // 向服务器发送请求报文
            if (request_size != write(client_sockfd, request, request_size))
            {
                // 如果发送的字节数与请求报文大小不一致，表示发送失败
                // cout << "2. 发送请求报文失败: " << strerror(errno) << endl; // 输出错误信息，strerror(errno) 用于获取错误描述
                failed_count++;       // 失败计数器增加
                close(client_sockfd); // 关闭套接字，释放该连接的资源

                client_sockfd = -1;         // 将套接字文件描述符设置为-1，表示当前无有效连接
                while (client_sockfd == -1) // 在连接无效的情况下循环，尝试重新建立连接
                {
                    client_sockfd = ConnectServer(server_host, server_port); // 尝试重新连接服务器，ConnectServer是一个假设存在的函数，用于建立到服务器的连接
                }
                continue; // 继续下一次循环，重试发送请求或处理其它任务
            }

            // cout << "2. 发送请求报文成功!" << endl;

            // 如果没有设置force，默认等待服务器的回复
            if (force == 0)
            {
                // keep-alive一个进程只创建一个套接字，收发数据都用这个套接字
                // 读取服务器返回的响应报文到response_buf中
                while (true)
                {
                    if (is_expired)
                    {
                        break;
                    }
                    int read_bytes = read(client_sockfd, response_buf, response_size);
                    if (read_bytes < 0)
                    {
                        // cout << "3. 接收响应报文失败: " << strerror(errno) << endl;
                        failed_count++;
                        close(client_sockfd); // 关闭套接字
                        // 读取响应失败，不用重新创建套接字，重新发一次请求即可
                        goto keep_alive;
                    }
                    else
                    {
                        // cout << "3. 接收响应报文成功！" << endl;
                        total_bytes += read_bytes;
                        break;
                    }
                }
            }
            success_count++;
            cout << "当前成功：" << success_count << ", 失败：" << failed_count << ", 总字节：" << total_bytes << endl;
        }
        
    }
    else
    {
    not_keep_alive:
        while (true)
        {
            if (is_expired)
            {
                // 收到闹钟信号，子进程工作结束
                if (failed_count > 0)
                {
                    failed_count--;
                }
                return;
            }
            // 与服务器建立连接
            client_sockfd = ConnectServer(server_host, server_port);
            if (client_sockfd < 0)
            {
                // cout << "1. 建立连接失败: " << strerror(errno) << endl;
                failed_count++;
                continue;
            }
            // cout << "1. 建立连接（TCP三次握手）成功!" << endl;

            // 向服务器发送请求报文
            if (request_size != write(client_sockfd, request, request_size))
            {
                // cout << "2. 发送请求报文失败: " << strerror(errno) << endl;
                failed_count++;
                close(client_sockfd); // 关闭套接字
                continue;
            }
            // cout << "2. 发送请求报文成功!" << endl;

            // HTTP 0.9特殊处理
            if (http_version == 0)
            {
                // 尝试关闭套接字的写入端
                if (shutdown(client_sockfd, 1))
                {
                    failed_count++;       // 如果关闭写入端失败，则增加失败计数
                    close(client_sockfd); // 关闭套接字，释放资源
                    continue;             // 继续执行循环的下一次迭代
                }
            }

            // 如果没有设置force，默认等待服务器的回复
            if (force == 0)
            {
                // not-keep-alive 每次发送完请求，读取了响应后就关闭套接字
                // 下次创建新套接字，发送请求，读取响应
                while (true)
                {
                    if (is_expired)
                    {
                        break;
                    }
                    int read_bytes = read(client_sockfd, response_buf, response_size);
                    if (read_bytes < 0)
                    {
                        // cout << "3. 接收响应报文失败: " << strerror(errno) << endl;
                        failed_count++;
                        close(client_sockfd); // 关闭套接字
                        // 这里读失败想退出来继续创建套接字去请求服务器，因为有两层循环，所以用goto
                        goto not_keep_alive;
                    }
                    else
                    {
                        // cout << "3. 接收响应报文成功！" << endl;
                        total_bytes += read_bytes;
                        break;
                    }
                }
            }

            // 一次发送接收完成后，关闭套接字
            if (close(client_sockfd))
            {
                // cout << "4. 关闭套接字失败: " << strerror(errno) << endl;
                failed_count++;
                continue;
            }
            success_count++;
            cout << "当前成功：" << success_count << ", 失败：" << failed_count << ", 总字节：" << total_bytes << endl;
        }
    }
}

// 命令行解析函数
void ParseArg(int argc, char *argv[])
{
    int opt = 0;           // 用于存储getopt_long返回的选项
    int options_index = 0; // 用于存储长选项的索引

    // 没有输入选项
    if (argc == 1)
    {
        Usage(); // 显示用法信息
        exit(1); // 退出程序
    }

    // 一个一个解析输入选项
    while ((opt = getopt_long(argc, argv, "fr912kt:c:p:Vh?",
                              OPTIONS, &options_index)) != EOF)
    {
        switch (opt)
        {
        case 'f':
            force = 1; // 不等待服务器响应
            break;
        case 'r':
            force_reload = 1; // 设置强制重新加载模式
            break;
        case '9':
            http_version = 0; // 设置HTTP版本为0.9
            break;
        case '1':
            http_version = 1; // 设置HTTP版本为1.0
            break;
        case '2':
            http_version = 2; // 设置HTTP版本为2.0
            break;
        case 'k':
            is_keep_alive = 1; // 设置Keep-Alive模式
            break;
        case 't':
            request_time = atoi(optarg); // 设置请求时间
            break;
        case 'c':
            clients_num = atoi(optarg); // 设置客户端数量
            break;
        case 'p':
        {
            // 使用代理服务器，设置其代理网络号和端口号
            char *proxy = strrchr(optarg, ':'); // 查找最后一个':'字符
            proxy_host = optarg;                // 设置代理主机
            if (proxy == NULL)
            {
                // 如果没有找到':'字符，说明格式不正确
                fprintf(stderr, "选项参数错误, 代理服务器%s: 缺少端口号\n", optarg);
                exit(1); // 退出程序
            }
            if (proxy == optarg)
            {
                // 如果':'字符在字符串开头，说明缺少主机名
                fprintf(stderr, "选项参数错误, 代理服务器%s: 缺少主机名\n", optarg);
                exit(1); // 退出程序
            }
            if (proxy == optarg + strlen(optarg) - 1)
            {
                // 如果':'字符在字符串末尾，说明缺少端口号
                fprintf(stderr, "选项参数错误, 代理服务器%s: 缺少端口号\n", optarg);
                exit(1); // 退出程序
            }
            *proxy = '\0';          // 将':'替换为'\0'，分割主机名和端口号
            port = atoi(proxy + 1); // 解析端口号，将其转换为整数
            break;
        }

        case '?':
        case 'h':
            Usage(); // 显示用法信息
            exit(0); // 退出程序
        case 'V':
            printf("WebBench %s\n", WEBBENCH_VERSION); // 显示版本信息
            exit(0);                                   // 退出程序
        default:
            Usage(); // 显示用法信息
            exit(1); // 退出程序
        }
    }

    // 没有输入URL
    if (optind == argc)
    {
        fprintf(stderr, "WebBench: 缺少URL参数!\n");
        Usage(); // 显示用法信息
        exit(1); // 退出程序
    }

    // 如果没有设置客户端数量和连接时间，则设置默认值
    if (clients_num == 0)
    {
        clients_num = 1; // 默认客户端数量为1
    }
    if (request_time == 0)
    {
        request_time = 30; // 默认请求时间为30秒
    }
}

// 构造请求
void BuildRequest(const char *url)
{
    bzero(host, MAXHOSTNAMELEN);      // 清空host数组
    bzero(request_buf, REQUEST_SIZE); // 清空request_buf数组

    // 无缓存和代理都要在HTTP1.0以上才能使用
    if (force_reload && proxy_host != NULL && http_version < 1)
    {
        http_version = 1; // 设置HTTP版本为1.0
    }

    // HEAD方法需要HTTP1.0以上版本
    if (request_method == METHOD_HEAD && http_version < 1)
    {
        http_version = 1; // 设置HTTP版本为1.0
    }

    // OPTIONS和TRACE方法需要HTTP1.1版本
    if (request_method == METHOD_OPTIONS && http_version < 2)
    {
        http_version = 2; // 设置HTTP版本为1.1
    }

    if (request_method == METHOD_TRACE && http_version < 2)
    {
        http_version = 2; // 设置HTTP版本为1.1
    }

    // 设置HTTP请求行的方法
    switch (request_method)
    {
    case METHOD_GET:
        strcpy(request_buf, "GET");
        break;
    case METHOD_HEAD:
        strcpy(request_buf, "HEAD");
        break;
    case METHOD_OPTIONS:
        strcpy(request_buf, "OPTIONS");
        break;
    case METHOD_TRACE:
        strcpy(request_buf, "TRACE");
        break;
    default:
        strcpy(request_buf, "GET");
        break;
    }
    strcat(request_buf, " ");

    // 判断URL是否有效
    if (NULL == strstr(url, "://"))
    {
        fprintf(stderr, "\n%s: 无效的URL\n", url);
        exit(2); // 退出程序
    }

    if (strlen(url) > 1500)
    {
        fprintf(stderr, "URL长度过长\n");
        exit(2); // 退出程序
    }

    if (proxy_host == NULL)
    {
        // 忽略大小写比较前7位
        if (0 != strncasecmp("http://", url, 7))
        {
            fprintf(stderr, "\n无法解析URL(只支持HTTP协议, 暂不支持HTTPS协议)\n");
            exit(2); // 退出程序
        }
    }

    // 定位URL中主机名开始的位置
    int i = strstr(url, "://") - url + 3;

    // 在主机名开始的位置是否有'/' 没有则无效
    if (strchr(url + i, '/') == NULL)
    {
        fprintf(stderr, "\n无效的URL,主机名没有以'/'结尾\n");
        exit(2); // 退出程序
    }

    // 填写请求URL 无代理时
    if (proxy_host == NULL)
    {
        // 有端口号 填写端口号
        if (index(url + i, ':') != NULL && index(url + i, ':') < index(url + i, '/'))
        {
            // 设置域名或IP
            strncpy(host, url + i, strchr(url + i, ':') - url - i);
            char port_str[10];
            bzero(port_str, 10);
            strncpy(port_str, index(url + i, ':') + 1, strchr(url + i, '/') - index(url + i, ':') - 1);
            // 设置端口
            port = atoi(port_str);
            // 避免写了: 却没写端口号
            if (port == 0)
            {
                port = 9190; // 设置默认端口80
            }
        }
        else
        {
            strncpy(host, url + i, strcspn(url + i, "/"));
        }
        strcat(request_buf + strlen(request_buf), url + i + strcspn(url + i, "/"));
    }
    else
    {
        // 有代理服务器 直接填就行
        strcat(request_buf, url);
    }

    // 填写HTTP协议版本
    if (http_version == 0)
    {
        strcat(request_buf, " HTTP/0.9");
    }
    else if (http_version == 1)
    {
        strcat(request_buf, " HTTP/1.0");
    }
    else if (http_version == 2)
    {
        strcat(request_buf, " HTTP/1.1");
    }
    strcat(request_buf, "\r\n");

    // 请求头部
    if (http_version > 0)
    {
        strcat(request_buf, "User-Agent: WebBench " WEBBENCH_VERSION "\r\n");
    }

    // 域名或IP字段
    if (proxy_host == NULL && http_version > 0)
    {
        strcat(request_buf, "Host: ");
        strcat(request_buf, host);
        strcat(request_buf, "\r\n");
    }

    // 强制重新加载，无缓存字段
    if (force_reload && proxy_host != NULL)
    {
        strcat(request_buf, "Pragma: no-cache\r\n");
    }

    // Keep-Alive
    if (http_version > 1)
    {
        strcat(request_buf, "Connection: ");
        if (is_keep_alive)
        {
            strcat(request_buf, "keep-alive\r\n");
        }
        else
        {
            strcat(request_buf, "close\r\n");
        }
    }

    // 添加空行
    if (http_version > 0)
    {
        strcat(request_buf, "\r\n");
    }

    printf("Request: %s\n", request_buf); // 打印请求报文
    PrintMessage(url);                    // 打印其他信息
}

// 父进程的作用: 创建子进程，读取子进程测试到的数据，然后处理
void WebBench()
{
    pid_t pid = 0;                      // 存储进程ID
    FILE *pipe_fd = NULL;               // 管道文件指针
    int req_succ, req_fail, resp_bytes; // 记录请求成功数、请求失败数和响应字节数

    // 尝试建立连接一次
    int sockfd = ConnectServer(proxy_host == NULL ? host : proxy_host, port);
    if (sockfd < 0)
    {
        fprintf(stderr, "\n连接服务器失败, 中断压力测试\n");
        exit(1); // 退出程序
    }
    close(sockfd); // 关闭套接字

    // 打印压测开始的消息
    // PrintMessage("http://127.0.0.1:9190/echo");

    // 创建父子进程通信的管道
    if (pipe(pipeline))
    {
        perror("通信管道建立失败");
        exit(1); // 退出程序
    }

    int proc_count;
    // 让子进程取测试，建立子进程数量由clients_num决定
    for (proc_count = 0; proc_count < clients_num; proc_count++)
    {
        // 创建子进程
        pid = fork();
        // <0是失败, =0是子进程, 这两种情况都结束循环 =0时子进程可能继续fork
        if (pid < 0)
        {
            fprintf(stderr, "第%d个子进程创建失败\n", proc_count);
            perror("创建子进程失败");
            exit(1); // 退出程序
        }
        if (pid == 0)
        {
            sleep(1); // 子进程休眠1秒
            break;
        }
    }

    // 子进程处理
    if (pid == 0)
    {
        close(pipeline[0]); // 子进程关闭管道的读端

        // 子进程创建客户端连接服务端，并发出请求报文
        Worker(proxy_host == NULL ? host : proxy_host, port, request_buf);
        pipe_fd = fdopen(pipeline[1], "w"); // 打开管道写端
        if (pipe_fd == NULL)
        {
            perror("管道写端打开失败");
            exit(1); // 退出程序
        }
        fprintf(stderr, "子进程 %d: 请求成功数: %d, 请求失败数: %d, 响应字节数: %d\n", getpid(), success_count, failed_count, total_bytes);
        // 进程运行完后，向管道写端写入该子进程在这段时间内请求成功的次数，失败的次数，读取到的服务器响应总字节数
        fprintf(pipe_fd, "%d %d %d\n", success_count, failed_count, total_bytes);
        // 关闭管道写端
        fclose(pipe_fd);
        exit(0); // 退出子进程
    }
    else
    {
        close(pipeline[1]); // 父进程关闭管道的写端

        // 父进程处理
        pipe_fd = fdopen(pipeline[0], "r"); // 打开管道读端
        if (pipe_fd == NULL)
        {
            perror("管道读端打开失败");
            exit(1); // 退出程序
        }
        // 因为我们要求数据要及时，所以没有缓冲区是最合适的
        setvbuf(pipe_fd, NULL, _IONBF, 0); // 设置无缓冲

        success_count = 0;
        failed_count = 0;
        total_bytes = 0;
        while (true)
        {
            // 从管道读端读取数据
            int result = fscanf(pipe_fd, "%d %d %d", &req_succ, &req_fail, &resp_bytes);
            if (result < 3)
            {
                fprintf(stderr, "某个子进程异常，返回值: %d\n", result);
                break;
            }
            success_count += req_succ;
            failed_count += req_fail;
            total_bytes += resp_bytes;
            fprintf(stderr, "父进程: 收到数据 - 请求成功数: %d, 请求失败数: %d, 响应字节数: %d\n", req_succ, req_fail, resp_bytes);
            // 我们创建了clients_num个子进程，所以要读clients_num多次数据
            if (--clients_num == 0)
            {
                break;
            }
        }
        fclose(pipe_fd); // 关闭管道读端
        printf("\n速度: %d 请求/秒, %d 字节/秒.\n请求: %d 成功, %d 失败\n",
               (int)((success_count + failed_count) / (float)request_time),
               (int)(total_bytes / (float)request_time),
               success_count, failed_count);
    }
}
