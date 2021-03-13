/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */
#ifndef BUFFER_H_
#define BUFFER_H_
#include <unistd.h>
#include <vector>
#include <atomic>
#include <assert.h>
#include <sys/uio.h>
#include <cstring>
#include <iostream>

class Buffer
{
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;
    //返回未读字节数
    size_t getReadableBytes() const;
    //返回可写字节数
    size_t getWritableBytes() const;
    //返回前置的未用空间
    size_t getPrependableBytes() const;
    //加减读位置
    void addReadPos(size_t len);
    //加减写位置
    void addWritePos(size_t len);
    //设置读位置
    void setReadPos(size_t pos);
    //设置写位置
    void setWritePos(size_t pos);
    //获取读指针
    char *getReadPtr() const;
    const char *getReadPtrConst() const;

    char *getBufferBeginPtr() const;
    const char *getBufferBeginPtrConst() const;
    //获取写指针
    const char *getWritePtrConst() const;
    char *getWritePtr() const;

    //清除缓冲区
    void clear();

    //Buffer扩容
    void getEnoughSpace(size_t len);
    //追加数据到Buffer
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const std::string &str);
    // 从fd中读取到Buffer
    ssize_t readBufferToFd(int fd, int *saveErrno);
    // 向fd中写入Buffer的内容
    ssize_t writeBufferfromFd(int fd, int *saveErrno);

private:
    //缓冲区，可作为读缓冲，也可作为写缓冲
    std::vector<char> Buffer_;
    //读写位置变量，每次进行运算都是原子操作
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};
#endif