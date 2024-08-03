#include"webbench.h"

// main函数
int main(int argc, char *argv[])
{
    // 解析命令行参数
    ParseArg(argc, argv);

    // 构建HTTP请求
    BuildRequest(argv[optind]);

    // 运行WebBench进行压力测试
    WebBench();

    return 0;
}
