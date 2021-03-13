/*
 * @Author       : zmingfeng
 * @Date         : 2021-03-11
 * @copyleft Apache 2.0
 */

#include "httpconnect.h"
/*
 * @brief                   httpConn构造
 * @param 
 * @return                  
 */
bool httpConn::isET;
const char *httpConn::rootDir;
std::atomic<int> httpConn::userCount;
httpConn::httpConn() : fd_(-1), addr_({0}), isClosed_(true) {}
void httpConn::Init(int fd, const sockaddr_in &addr)
{
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;

    writeBuffer_.clear();
    readBuffer_.clear();

    isClosed_ = false;
}
httpConn::~httpConn()
{
    Close();
}

void httpConn::Close()
{
    response_.unMapfile();
    if (!isClosed_)
    {
        isClosed_ = true;
        userCount--;
        close(fd_);
    }
}

int httpConn::getFd() const
{
    return fd_;
}

struct sockaddr_in httpConn::getAddr() const
{
    return addr_;
}

const char *httpConn::getIP() const
{
    //将主机地址转化为字符串形式
    return inet_ntoa(addr_.sin_addr);
}
int httpConn::getPort() const
{
    return addr_.sin_port;
}

ssize_t httpConn::read(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = readBuffer_.writeBufferfromFd(fd_, saveErrno);
        if (len <= 0)
            break;

    } while (isET);
    return len;
}

ssize_t httpConn::write(int *saveErrno)
{
    //这里不直接用写缓冲区的原因是，写缓冲区一般填写头部，而数据部分会用另一个临时缓冲区存储
    ssize_t len = -1;
    do
    {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0)
        {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0)
        {
            break;
        }
        else if (len > static_cast<ssize_t>(iov_[0].iov_len))
        {
            iov_[1].iov_base = (char *)(iov_[1].iov_base) + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                writeBuffer_.addWritePos(iov_[0].iov_len);
                iov_[0].iov_base = writeBuffer_.getWritePtr();
                iov_[0].iov_len = 0;
            }
        }
        else
        {
            writeBuffer_.addWritePos(len);
            iov_[0].iov_base = writeBuffer_.getWritePtr();
            iov_[0].iov_len -= len;
        }
    } while (isET);
    return len;
}

bool httpConn::process()
{
    request_.Init();
    if (readBuffer_.getReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuffer_))
    {
        response_.Init(rootDir, request_.getPath(), request_.isKeepAlive(), 200);
    }
    else
    {
        response_.Init(rootDir, request_.getPath(), false, 400);
    }

    response_.makeResponse(writeBuffer_);
    iov_[0].iov_base = const_cast<char *>(writeBuffer_.getReadPtrConst());
    iov_[0].iov_len = writeBuffer_.getReadableBytes();
    iovCnt_ = 1;

    if (response_.getFileLength() > 0 && response_.getFile())
    {
        iov_[1].iov_base = response_.getFile();
        iov_[1].iov_len = response_.getFileLength();
        iovCnt_ = 2;
    }
    return true;
}

ssize_t httpConn::getToWriteBytes()
{
    return iov_[1].iov_len + iov_[0].iov_len;
}

bool httpConn::isKeepAlive() const
{
    return request_.isKeepAlive();
}