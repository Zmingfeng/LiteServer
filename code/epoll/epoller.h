#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h>
#include <vector>
#include <assert.h>
#include <unistd.h>

class Epoller
{
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
    int wait(int timeoutMs);
    int getEventFd(size_t idx) const;
    uint32_t getEvent(size_t idx) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};

#endif