#include "epoller.h"

Epoller::Epoller(int maxEvents) : epollFd_(epoll_create(maxEvents)), events_(maxEvents)
{
    assert(epollFd_ >= 0 && events_.size() > 0);
}
Epoller::~Epoller()
{
    //关闭epollFd这个特殊的句柄
    close(epollFd_);
}

bool Epoller::addFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    //向内核注册该句柄，放到红黑树上
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd)
{
    if (fd < 0)
        return false;
    struct epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}
/*
 * @brief                   //等待readylist非空
 * @param                   timeoutMs为0会立即返回，为-1则阻塞等待事件准备好
 * @return                  返回ready事件数， 返回0表示已超时
 */
int Epoller::wait(int timeoutMs)
{
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::getEventFd(size_t idx) const
{
    assert(idx >= 0 && idx < events_.size());
    return events_[idx].data.fd;
}

uint32_t Epoller::getEvent(size_t idx) const
{
    assert(idx >= 0 && idx < events_.size());
    return events_[idx].events;
}
