/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */

#include "buffer.h"
/*
 * @brief                   Buffer构造函数  
 * @param initBufferSize    设置的BufferSize大小，在声明时使用了默认参数，这里就不用写成默认参数的形式了
 * @param 
 * @return  
 */
Buffer::Buffer(int initBufferSize) : Buffer_(initBufferSize), readPos_(0), writePos_(0) {}
/*
 * @brief                   获取可读字节数  
 * @param 
 * @return                  unsigned long 类型的整数
 */
size_t Buffer::getReadableBytes() const
{
    return writePos_ - readPos_;
}
/*
 * @brief                   获取可写字节数  
 * @param 
 * @return                  unsigned long 类型的整数
 */
size_t Buffer::getWritableBytes() const
{
    return Buffer_.size() - writePos_;
}
/*
 * @brief                   获取前置的已读空间
 * @param 
 * @return                  unsigned long 类型的整数
 */
size_t Buffer::getPrependableBytes() const
{
    return readPos_;
}
/*
 * @brief                   增减读指针
 * @param 
 * @return                  空
 */
void Buffer::addReadPos(size_t len)
{
    readPos_ += len;
}
/*
 * @brief                   增减写指针
 * @param 
 * @return                  空
 */
void Buffer::addWritePos(size_t len)
{
    writePos_ += len;
}
/*
 * @brief                   设置读指针
 * @param 
 * @return                  空
 */
void Buffer::setReadPos(size_t pos)
{
    readPos_ = pos;
}
/*
 * @brief                   设置写指针
 * @param 
 * @return                  空
 */
void Buffer::setWritePos(size_t pos)
{
    writePos_ = pos;
}
char *Buffer::getReadPtr() const
{
    return (char *)&(*Buffer_.begin()) + readPos_;
}
/*
 * @brief                   获取读指针
 * @param 
 * @return                  常量指针
 */
const char *Buffer::getReadPtrConst() const
{
    return (char *)&(*Buffer_.begin()) + readPos_;
}
/*
 * @brief                   获取写指针
 * @param 
 * @return                  常量指针
 */
const char *Buffer::getWritePtrConst() const
{
    return (char *)&(*Buffer_.begin()) + writePos_;
}
/*
 * @brief                   获取写指针
 * @param 
 * @return                  指针
 */
char *Buffer::getWritePtr() const
{
    return (char *)&(*Buffer_.begin()) + writePos_;
}
/*
/*
 * @brief                   获取Buffer起始地址
 * @param 
 * @return                  常量指针，指向Buffer头
 */
char *Buffer::getBufferBeginPtr() const
{
    return (char *)&(*Buffer_.begin());
}
const char *Buffer::getBufferBeginPtrConst() const
{
    return (char *)&(*Buffer_.begin());
}
void Buffer::clear()
{
    bzero(&Buffer_[0], Buffer_.size());
    setWritePos(0);
    setReadPos(0);
}
void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}
/*
 * @brief                   将len长度的字符数组中的内容追加到buffer末尾
 * @param 
 * @return                  
 */
void Buffer::Append(const void *data, size_t len)
{
    assert(data);
    Append(static_cast<const char *>(data), len);
}
void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    getEnoughSpace(len);
    std::copy(str, str + len, getWritePtr());
    addWritePos(len);
}
/*
 * @brief                   Buffer扩容
 * @param 
 * @return                  
 */
void Buffer::getEnoughSpace(size_t len)
{
    int writableSize = getWritableBytes();
    if (writableSize < len)
    {
        if (writableSize + getPrependableBytes() < len)
        {
            //增加len个单元
            Buffer_.resize(writePos_ + len);
        }
        else
        {
            //未读内容前移，空出后面的空间
            size_t readableSize = getReadableBytes();
            std::copy(getReadPtrConst(), getWritePtrConst(), getBufferBeginPtr());
            setReadPos(0);
            setWritePos(readableSize);
        }
    }
}
/*
 * @brief                   从句柄中读出数据到Buffer
 * @param 
 * @return                  读的字节数
 */
ssize_t Buffer::writeBufferfromFd(int fd, int *saveErrno)
{
    char tempBuff[65535]; //65535就一定够用呢
    //自定义IO缓冲区，每个缓冲区都可以设置基址和长度
    struct iovec iov[2];
    // 获取Buffer剩余可写字节
    const size_t writableSize = getWritableBytes();

    iov[0].iov_base = getWritePtr();
    iov[0].iov_len = writableSize;
    iov[1].iov_base = tempBuff;
    iov[1].iov_len = 65535;
    //从iov[0]到iov[1]按序填充，返回读取到的字节数
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *saveErrno = errno; //应该是readv时设置的错误位
    }
    // 这里表明没用到第二个缓冲区
    else if (static_cast<size_t>(len) <= writableSize)
    {
        //移动写指针到指定位置
        addWritePos(len);
    }
    else
    {
        //设置写指针，还要继续读没写完的
        setWritePos(Buffer_.size());
        Append(tempBuff, len - writableSize);
    }
    return len;
}

/*
 * @brief                   从Buffer读出数据
 * @param 
 * @return                  写入的字节数
 */
ssize_t Buffer::readBufferToFd(int fd, int *saveErrno)
{
    size_t readSize = getReadableBytes();
    ssize_t len = write(fd, getReadPtrConst(), readSize);
    if (len < 0)
    {
        //写入错误
        *saveErrno = errno;
        return len;
    }
    addReadPos(len);
    return len;
}