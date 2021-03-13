/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../http/httpconnect.h"
#include "../pools/sqlpool.h"
#include "../epoll/epoller.h"
#include "../pools/threadpool.h"
#include "../timer/timer.h"
///    WebServer类定义
///    所有的方法都在webserver.cpp文件中进行方法体定义
class WebServer
{
public:
    WebServer(int port, int trigerMode, int timeoutMs, bool optLinger,
              int sqlport, const char *sqlUser, const char *sqlPwd,
              const char *dbName, int sqlPoolSize, int threadPoolSize);
    ~WebServer();
    void start();
    void setNonBlock(int fd);
    void onProcess(httpConn *client);
    static const int MAX_FD = 65536;

private:
    //socket 连接端口号
    int port_;
    // 触发模式 ET/LT
    int trigerMode_;
    int listenEvent_;
    int connEvent_;
    // 超时时间
    int timeoutMs_;
    // 是否选择优雅退出
    bool optLinger_;
    // 连接是否关闭
    bool isClosed_;
    //监听的句柄
    int listenFd_;
    // 根目录
    char *rootDir_;
    // unique_ptr好处在于能防止内存泄漏及交叉指向问题，整个项目这三个变量都是唯一的
    std::unique_ptr<Timer> Timer_;
    std::unique_ptr<threadPool> threadPool_;
    std::unique_ptr<Epoller> Epoller_;
    //不同的句柄对应不同的客户端连接，需要为每个句柄分配一个连接实例
    std::unordered_map<int, httpConn> Users_;

    void InitEvenMode_(int trigerMode);
    bool InitSocket_();
    void extentTime_(httpConn *conn);
    void dealWrite_(httpConn *client);
    void dealRead_(httpConn *client);

    void closeConn_(httpConn *clinet);
    void sendError_(size_t fd, const std::string message);
    void dealListen_();
    void addClient_(size_t fd, const sockaddr_in &addr);
    void Write_(httpConn *client);
    void Read_(httpConn *client);
};

#endif