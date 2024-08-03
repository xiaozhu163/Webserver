# Makefile
# $^ 代表所有依赖⽂件
# $@ 代表所有⽬标⽂件
# $< 代表第⼀个依赖⽂件
# % 代表通配符

# Makefile的变量定义
TARGET := web_bench             # 最终生成的可执行文件名
STATIC_LIB := libweb_bench.a    # 静态库文件名
SHARED_LIB := libweb_bench.so   # 动态库文件名
CXX := g++                      # C++编译器
CXXFLAGS := -std=gnu++11 -Wfatal-errors -Wno-unused-parameter  # 编译选项
LDFLAGS := -Wl,-rpath=./lib     # 链接选项
INC_DIR := -I ./include         # 头文件目录
LIB_DIR := -L ./                # 库文件目录
LIBS := -lpthread               # 链接库
DEBUG := 0                      # 调试开关，0为关闭，1为打开

# 根据DEBUG变量的值设置不同的编译选项
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -DDEBUG
else
    CXXFLAGS += -O3 -DNDEBUG
endif

# 默认目标，编译静态库和可执行文件
all: $(STATIC_LIB) $(TARGET)

# 源文件
SOURCES := web_bench.cpp

# 所有.cpp源文件对应的.o目标文件
OBJS := $(patsubst %.cpp, %.o, $(SOURCES))

# 主程序的源文件和目标文件
MAIN := main.cpp
MAIN_OBJ := main.o

# 编译规则，将.cpp文件编译成.o文件
%.o: %.cpp
    @echo -e '\e[1;32mBuilding CXX object: $@\e[0m'
    $(CXX) -c $^ -o $@ $(CXXFLAGS) $(INC_DIR)

# 链接生成静态库
$(STATIC_LIB): $(OBJS)
    @echo -e '\e[1;36mLinking CXX static library: $@\e[0m'
    ar -rcs -o $@ $^
    @echo -e '\e[1;35mBuilt target: $@\e[0m'

# 链接生成动态库
$(SHARED_LIB): $(OBJS)
    @echo -e '\e[1;36mLinking CXX shared library: $@\e[0m'
    $(CXX) -fPIC -shared -o $@ $^
    @echo -e '\e[1;35mBuilt target: $@\e[0m'

# 链接生成可执行文件
$(TARGET): $(MAIN_OBJ) $(STATIC_LIB)
    @echo -e '\e[1;36mLinking CXX executable: $@\e[0m'
    $(CXX) -o $@ $^ $(LDFLAGS)
    @echo -e '\e[1;35mBuilt target: $@\e[0m'

# 声明伪目标文件
.PHONY: all clean

# 安装规则，将生成的静态库和可执行文件移动到相应的目录
install: $(STATIC_LIB) $(TARGET)
    @if (test ! -d lib); then mkdir -p lib; fi
    @mv $(STATIC_LIB) lib
    @if (test ! -d bin); then mkdir -p bin; fi
    @mv $(TARGET) bin

# 清理规则，删除生成的中间文件和目标文件
clean:
    rm -f $(OBJS) $(MAIN_OBJ) $(TARGET) $(STATIC_LIB) $(SHARED_LIB) ./bin/$(TARGET) ./lib/$(STATIC_LIB) ./lib/$(SHARED_LIB)
