# Webserver
Tiny Webserver

![1722684437001](image/README/1722684437001.png)


---

### WebServer 设计与实现

#### 主函数
- **功能**：
  - 输入端口号、子反应堆线程数、日志路径。
  - 输入 MySQL 数据库登录的用户名和密码，用于验证用户身份。
  - 设置日志文件的路径。

- **日志系统**：
  - **Logstream**：将各类型数据转换成字符形式并放入字符数组中（bufferA）。每个 Logstream 仅持有一个缓冲区 buffer，重载各种形式的 `<<` 操作符。
  - **AsyncLogging**：异步日志类，负责将日志数据放入后端缓冲区，使用双缓冲区方法。创建两个 large buffer，一个 bufferA 用于接收日志信息，异步记录器在条件变量满足或计时器超时时将 bufferA 内容写入 buffer_ 队列中，然后 bufferA 被移入备用的新 buffer。buffer_ 队列中的内容被写入 Logfile 对应的内核文件缓冲区，达到一定次数时 flush 进文件中。
  - **Logging**：封装了 Logstream，每个 LOG 使用地方都会创建一个 Logger 持有一个 Logstream，语句结束时析构函数将当前 buffer 内容写入异步日志类。
  - **Logfile**：提供 append 和 flush 函数，将内容写入内核文件缓冲区以及文件中。

#### 计数器
- **功能**：维护一个数值和条件变量，数值大于0时一直等待。调用一次 `countDown` 数值减1，到0时 `wait` 函数阻塞解除，继续执行。用于确保日志线程进入回调函数后主线程才会继续执行（`wait` 解除）。

#### 内存池
- **功能**：
  - 创建 8、16、...、512 共 64 个内存池数组。
  - 每个内存池中存储大小为 4096 的块，块内部分为不同的 slot。
  - 每个 slot 是一个结构体，内部存储一个指针，用于保存 `next` 节点。使用时强转成 T 类型，不使用时转成 slot 类型形成链表待命。

#### LFU 缓存
- **功能**：
  - 初始化有一个缓存实例（static），保证缓存模块内部存储的键值对一致性。
  - 包含两个哈希表，一个存储 key 对应频率链表的头结点（初始化时创建频度0结点为哨兵结点），另一个存储每个插入的 key 对应的小链表结点。
  - 插入键值时，根据大表验证是否存在：若存在则从当前小表中取下放到下一个小频率表，并更新大表 key 对应的表头。若对应 +1 表不存在，则创建 +1 大表和小链表。

#### 获取主循环 EventLoop -- mainloop
- **功能**：
  - **定时器模块**：
    - 包括定时器管理器和定时器，每个 EventLoop 有一个定时器管理器，每个连接对应一个 HttpData（创建对应的 channel），关联一个定时器。
    - 定时器管理器在每次红黑树发生变化时添加新的定时器，不进行新的替换旧的操作。处理每个 channel 的事件时，处理完读写后处理连接事件函数，分离当前定时器，更新红黑树，添加新的定时器到该 channel。
    - 连接关闭时 HttpData 会设置关联的定时器为 delete 状态。旧的定时器在到达一定条件时不断删除。

  - **EventLoop**：
    - 每个 EventLoop 有一个守候套接字来启动循环，设置超时时间为0以异步唤醒 loop 的 wait 状态。
    - 主 loop 的监听套接字设置超时时间为0。

#### 创建 WebServer 实例
- **功能**：
  - 在内存池上申请空间创建一个 WebServer 实例。
  - 主 loop 线程负责接收新的连接请求，套接字按照轮训算法分发给每个子 loop 线程。
  - Server 模块创建主循环 loop，设置子线程数，线程池，激活监听套接字通道，设置非阻塞模式，处理 SIGPIPE 信号。

#### EventLoopThreadPool 模块
- **功能**：
  - 提供启动每个线程的函数以及轮询的函数，启动线程后保存到 EventLoop 容器和 EventLoopThread 容器。

- **EventLoopThread 模块**：
  - 封装线程和 EventLoop，创建线程的启动函数及线程的回调函数（运行 loop）。

#### 并发模块
- **功能**：
  - **EventLoop**：
    - 封装了 epoll 和 loop 函数，处理待执行的回调函数队列，使用 wakeup 套接字用于异步唤醒。
  - **Channel**：
    - 封装了一个 fd 和感兴趣事件，处理该 fd 上的事件。提供设置 fd 的感兴趣事件、获取 channel 感兴趣事件的函数、设置事件发生时的处理函数、以及 handleEvent 函数处理事件。
  - **Epoll**：
    - 封装了红黑树操作函数、epoll_wait 函数、定时器超时处理函数。负责监听文件描述符事件并返回触发事件的文件描述符。一个 Epoll 对象对应一个 IO 多路复用模块，一个 EventLoop 对应一个 Epoll。

  - **Thread 模块**：
    - 封装线程的 start 和 join 函数，创建线程的 data 结构体保存线程信息，利用计数器锁存器保证线程启动后主线程继续执行。

#### 主 Loop 处理新连接
- **功能**：
  - 启动 loop 对应的 Epoll 的 poll 函数，监听新连接。获取套接字后轮询子反应堆获取一个子 loop，设置套接字属性为非阻塞、Nodelay。
  - 创建 HttpData 封装套接字，创建 channel 并关联。将 HttpData 的 newEvent 函数放入 loop 的待处理连接函数队列中。

#### 获取数据库连接池实例，连接数据库
- **功能**：
  - 数据库连接池初始化，预先创建连接池列表，保存设置数量的数据库连接。
  - 使用单例模式获取连接，执行 MySQL 指令验证用户身份。
  - 使用信号量防止并发连接超出连接池上限。

#### 启动服务器 + 启动数据库
- **功能**：启动服务器和数据库服务，开始主事件循环。

#### 主事件循环
- **功能**：
  - 主事件的红黑树不断接收新连接，从事件 loop 不断接收主事件分发的套接字，监听这些套接字的动作（读、请求消息、分析 HTTP 报文、分析头、分析请求行、处理请求、封装应答、写回应答）。

#### 结束主事件循环，释放资源
- **功能**：结束主事件循环，删除主事件循环对象，释放资源。

#### 性能测试
- **LFU 短连接**：
  - 测试 LFU 缓存下的短连接性能。
  - **结果**：速度: 15645 请求/秒，1658370 字节/秒。请求: 938701 成功，173 失败。

- **LFU 长连接**：
  - 测试 LFU 缓存下的长连接性能。
  - **结果**：速度: 64119 请求/秒，10130829 字节/秒。请求: 3847156 成功，0 失败。

- **无 LFU 短连接**：
  - 测试不使用 LFU 缓存下的短连接性能。
  - **结果**：速度: 1787 请求/秒，2499625 字节/秒。请求: 107267 成功，259 失败。

- **无 LFU 长连接**：
  - 测试不使用 LFU 缓存下的长连接性能。
  - **结果**：速度: 2590 请求/秒，3621956 字节/秒。请求: 155395 成功，0 失败。

#### CPU 占用
- **功能**：测量服务器在不同负载下的 CPU 使用情况。

![1722685604667](image/README/1722685604667.png)

---
