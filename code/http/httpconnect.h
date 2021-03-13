/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */
#ifndef HTTPCONNECT_H
#define HTTPCONNECT_H
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <atomic>
#include "request.h"
#include "response.h"
#include "../buffer/buffer.h"

//处理http连接的类
class httpConn
{
public:
    httpConn();
    ~httpConn();
    void Init(int socketFd, const sockaddr_in &addr);
    //关闭连接，进行一系列处理
    void Close();
    int getFd() const;
    struct sockaddr_in getAddr() const;
    const char *getIP() const;
    int getPort() const;

    //从fd中读取数据到readBuffer
    ssize_t read(int *saveErrno);
    //写数据到writeBuffer
    ssize_t write(int *saveErrno);
    ssize_t getToWriteBytes();
    bool isKeepAlive() const;
    //读取数据并处理
    bool process();

    static bool isET;
    static const char *rootDir;
    static std::atomic<int> userCount;

private:
    // http连接句柄，每个http连接都是独立的
    int fd_;
    // socket通信相关的数据结构， ipv4地址 + 端口号
    struct sockaddr_in addr_;
    //http连接是否关闭
    bool isClosed_;
    //IO缓冲区的数量
    int iovCnt_;
    //使用两个io缓冲区
    struct iovec iov_[2];
    //读缓冲区
    Buffer readBuffer_;
    //写缓冲区
    Buffer writeBuffer_;

    httpRequest request_;
    httpResponse response_;
};

#endif