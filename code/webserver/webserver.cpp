/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */
#include "webserver.h"

/*
 * @brief               webserver类构造函数  
 * @param port          用于socket连接的端口号，需要在1000~65535之间，低数字端口是给内核中的程序使用的
 * @param trigerMode    触发模式，支持 ET/LT，使用ET效率更高，支持更高的并发效率
 * @param timeoutMS     超时时间，用于清理非活动http连接，防止不断对该句柄进行查询，提升效率
 * @param OptLinger     是否选择优雅退出，socket选项，指定函数close对面向连接的协议如何操作（如TCP）, 用setsocketopt(...)进行设置
 * @param 
 * @return  
 */

WebServer::WebServer(int port, int trigerMode, int timeoutMs, bool optLinger,
                     int sqlport, const char *sqlUser, const char *sqlPwd,
                     const char *dbName, int sqlPoolSize, int threadPoolSize) : Epoller_(new Epoller()), Timer_(new Timer()), threadPool_(new threadPool(threadPoolSize))
{
    port_ = port;
    trigerMode_ = trigerMode;
    timeoutMs_ = timeoutMs;
    optLinger_ = optLinger;
    isClosed_ = false;

    //获取当前工作目录绝对路径，内部会进行malloc，析构时应当free
    rootDir_ = getcwd(nullptr, 256);
    assert(rootDir_);
    //连接两个字符串，限制16个字符
    strncat(rootDir_, "/resources/", 16);
    //初始化http连接参数
    httpConn::userCount = 0;
    httpConn::rootDir = rootDir_;
    //初始化数据库连接池
    sqlPool::getInstance()->Init("localhost", sqlport, sqlUser, sqlPwd, dbName, sqlPoolSize);
    //初始化时间模式
    InitEvenMode_(trigerMode);
    //初始化socket，失败则关闭连接
    if (!InitSocket_())
        isClosed_ = true;
}
WebServer::~WebServer()
{
    close(listenFd_);
    isClosed_ = true;
    free(rootDir_);
    sqlPool::getInstance()->closePool();
}

void WebServer::start()
{
    //超时-1表示没有事件就阻塞
    int timeMs = -1;
    if (!isClosed_)
    {
        std::cout << "======================= server start ========================" << std::endl;
    }
    while (!isClosed_)
    {
        if (timeoutMs_ > 0)
        {
            timeMs = Timer_->getNextTick(); //这里会获取下一个超时时间，timeMs 毫秒，也是在这里清理非活动连接
        }
        //timeMs : -1则会一直阻塞，0会立即返回，等于说一直循环，>0则是超时返回，比较均衡，也就是说会到下一个连接变为非活动时返回
        size_t eventCnt = Epoller_->wait(timeMs);
        for (int i = 0; i < eventCnt; ++i)
        {
            size_t fd = Epoller_->getEventFd(i);
            uint32_t events = Epoller_->getEvent(i);
            if (fd == listenFd_)
            {
                dealListen_();
            }
            else if (events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
            {
                assert(Users_.count(fd) > 0);
                closeConn_(&Users_[fd]);
            }
            else if (events & EPOLLIN)
            { //有数据可读
                assert(Users_.count(fd) > 0);
                dealRead_(&Users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(Users_.count(fd) > 0);
                dealWrite_(&Users_[fd]);
            }
            else
                std::cout << "error!" << std::endl;
        }
    }
}
void WebServer::dealListen_()
{
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);
    do
    {
        size_t fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if (fd < 0)
            return;
        else if (httpConn::userCount >= MAX_FD)
        {
            sendError_(fd, "busy!");
            return;
        }
        else
        {
            addClient_(fd, addr);
        }
    } while (listenEvent_ & EPOLLET); //只有TCP就绪队列没有数据了才跳出，否则会出现高并发时很多连接连不上的问题
}

void WebServer::sendError_(size_t fd, const std::string message)
{
    int ret = send(fd, &message, message.size(), 0);
    if (ret < 0)
    {
        std::cout << "send failed!" << std::endl;
    }
    close(fd);
}
void WebServer::addClient_(size_t fd, const sockaddr_in &addr)
{
    assert(fd >= 0);
    Users_[fd].Init(fd, addr);
    if (timeoutMs_ > 0)
    {
        Timer_->add(fd, timeoutMs_, std::bind(&WebServer::closeConn_, this, &Users_[fd]));
    }
    Epoller_->addFd(fd, connEvent_ | EPOLLIN);
    setNonBlock(fd);
}
void WebServer::closeConn_(httpConn *client)
{
    assert(client);
    Epoller_->delFd(client->getFd());
    client->Close();
}
void WebServer::InitEvenMode_(int trigerMode)
{
    //初始化成读关闭状态
    listenEvent_ = EPOLLRDHUP;
    //初始化为one_shot模式,此时一次提醒之后就会清楚readylist，读取完成后需要再次修改为one_shot
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigerMode)
    {
    //默认都为LT模式
    case 0:
        break;
    //监听事件初始化为ET模式
    case 1:
        listenEvent_ |= EPOLLET;
        break;
    //连接事件设为ET模式
    case 2:
        connEvent_ |= EPOLLET;
        break;
    //监听和连接事件都为ET模式
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
    }
    httpConn::isET = (connEvent_ & EPOLLET);
}

bool WebServer::InitSocket_()
{
    int ret;
    struct sockaddr_in addr;
    if (port_ < 1000 || port_ > 65535)
    {
        std::cout << "port error(1000 - 65535) !";
        return false;
    }
    //设置服务器socket的地址，INADDR_ANY表示任意
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    struct linger optLinger = {0};
    if (optLinger_)
    {
        optLinger.l_linger = 1;
        optLinger.l_onoff = 1;
    }
    //根据协议簇返回一个socket句柄
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        std::cout << "create socket failed!" << std::endl;
    }
    //设置优雅退出socket选项
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(listenFd_);
        std::cout << "init linger failed!" << std::endl;
        return false;
    }
    // 用于表示地址复用，0表示不可以，1表示可以
    int optval = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval));
    //设置完了套接字选项，开始对socket进行命名绑定
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        close(listenFd_);
        std::cout << "bind addr error!" << std::endl;
        return false;
    }
    //开始监听，设置TCP最大监听队列，
    ret = listen(listenFd_, 6);
    if (ret < 0)
    {
        close(listenFd_);
        std::cout << "set listen error!" << std::endl;
        return false;
    }
    //设置IO复用
    ret = Epoller_->addFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0) //即返回false
    {
        close(listenFd_);
        std::cout << "add listen error!" << std::endl;
        return false;
    }
    //设置监听socket的句柄为非阻塞模式，也就是read()读完之后直接返回，不会阻塞
    //如果不设置非阻塞的话就会要么损失数据，要么就做不到高并发了
    //调用wait的时候，就会将readylist复制到events_数组中
    setNonBlock(listenFd_);
    return true;
}

void WebServer::setNonBlock(int fd)
{
    assert(fd >= 0);
    fcntl(listenFd_, F_SETFL, fcntl(listenFd_, F_GETFD, 0) | O_NONBLOCK);
}
void WebServer::dealRead_(httpConn *client)
{
    assert(client);
    extentTime_(client);
    threadPool_->addTask(std::bind(&WebServer::Read_, this, client));
}
void WebServer::dealWrite_(httpConn *client)
{
    assert(client);
    extentTime_(client);
    threadPool_->addTask(std::bind(&WebServer::Write_, this, client));
}

void WebServer::Read_(httpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno); //从fd读到buffer
    //读出问题了，而且不是内核缓冲区空的原因
    if (ret <= 0 && readErrno != EAGAIN)
    {
        closeConn_(client);
        return;
    }
    onProcess(client);
}

void WebServer::onProcess(httpConn *client)
{
    if (client->process())
    //如果是写入writebuffer成功，则要设置监听fd写事件（只要从内核缓冲区读出了数据应该没问题，只要内核缓冲区有空间可写就会直接触发）
    //，接下来将会调用逻辑单元写入fd
    {
        Epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
    }
    else //如果没有readbuffer没有数据，就要等待读
    {
        Epoller_->modFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}
void WebServer::Write_(httpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);   //写入fd，当内核缓冲区可写的时候，writebuffer不一定有数据可写
    if (client->getToWriteBytes() == 0) //如果buffer里面没有数据
    {
        if (client->isKeepAlive())
        {
            onProcess(client); //读取内核缓冲区的request数据并且写入response
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN) //内核缓冲区满，则等待客户端读完之后再写
        {
            Epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    //如果出问题了就关闭连接
    closeConn_(client);
}
void WebServer::extentTime_(httpConn *conn)
{
    assert(conn);
    if (timeoutMs_ > 0)
    {
        Timer_->adjust(conn->getFd(), timeoutMs_);
    }
}