#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>

class threadPool
{
public:
    threadPool(size_t maxthreadPoolSize = 10);
    threadPool(const threadPool &) = default;
    ~threadPool();
    //声明为模板的话，传入任何类型的右值都是可以的，不够安全
    // template <class F>
    //&&表示右值引用，也就是说传入的函数是不可以被替换掉的
    void addTask(std::function<void()> &&func);

private:
    int maxthreadPoolSize_;

    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };

    std::shared_ptr<Pool> pool_;
};

#endif