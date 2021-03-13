#ifndef TIMER_H
#define TIMER_H
#include <unistd.h>
#include <unordered_map>
#include <functional>
#include <time.h>
#include <chrono>
#include <vector>
#include <assert.h>

typedef std::function<void()> timeoutCallBack;    //超时回调函数
typedef std::chrono::high_resolution_clock Clock; //system clock, 默认是单位纳秒
typedef std::chrono::microseconds Ms;             //单位毫秒
typedef Clock::time_point timeStamp;              //时间戳，纳秒级别的

struct timerNode
{
    int id;
    timeStamp expires;
    timeoutCallBack cb;
    bool operator<(const timerNode &t)
    {
        return expires < t.expires;
    }
};
class Timer
{
public:
    Timer() { heap_.reserve(64); }; //堆中预留足够的空间
    ~Timer() { clear(); };
    void add(int id, int timeOut, const timeoutCallBack &cb);
    void doWork(int id);
    void del_(int idx);
    void adjust(int id, int timeOut);

    void clear();
    void pop();
    void tick();
    int getNextTick();

private:
    bool siftUp_(size_t idx);
    bool siftDown_(size_t idx, size_t n);
    void swapNode_(size_t idx1, size_t idx2);
    //以时间戳排序，小根堆
    std::vector<timerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};
#endif