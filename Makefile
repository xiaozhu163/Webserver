# 定义编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++11 -g -Wall -O3 -D_PTHREADS

# 定义目标文件和依赖文件
OBJS = Channel.o Epoll.o EventLoop.o EventLoopThread.o EventLoopThreadPool.o HttpData.o Server.o Timer.o Util.o \
       base/AsyncLogging.o base/CountDownLatch.o base/FileUtil.o base/LFUCache.o base/LogFile.o base/Logging.o \
       base/LogStream.o base/MemoryPool.o base/Thread.o CGImysql/sql_connection_pool.o Main.o

# 目标文件
TARGET = WebServer
LOGGING_TEST = LoggingTest

# 库文件
LIBS = -lpthread -L/usr/local/mysql/lib -lmysqlclient

# 编译目标
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(LOGGING_TEST): $(OBJS) base/tests/LoggingTest.o
	$(CXX) $(CXXFLAGS) -o $(LOGGING_TEST) $(OBJS) base/tests/LoggingTest.o $(LIBS)

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET) $(LOGGING_TEST)
