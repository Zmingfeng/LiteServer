#include "threadpool.h"

threadPool::threadPool(size_t maxthreadPoolSize) : pool_(std::make_shared<Pool>())
{
    assert(maxthreadPoolSize > 0);
    for (size_t i = 0; i < maxthreadPoolSize; ++i)
    {
        std::thread([pool = pool_] {
            //互斥锁加锁
            std::unique_lock<std::mutex> locker(pool->mtx);
            while (true)
            {
                if (!pool->tasks.empty())
                {
                    //渠道第一个任务
                    auto task = std::move(pool->tasks.front());
                    pool->tasks.pop();
                    //解锁，只有在竞争任务资源的时候需要加锁
                    locker.unlock();
                    task();
                    //一般加锁
                    locker.lock();
                }
                else if (pool->isClosed)
                    break; //任务列表为空的时候坚持一下是否关闭了线程池，是的话就退出线程
                else
                    //条件变量+互斥锁，用于获取任务资源
                    pool->cond.wait(locker);
            }
        }).detach();
    }
}
threadPool::~threadPool()
{
    if (static_cast<bool>(pool_))
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->isClosed = true;
        }
        pool_->cond.notify_all(); //执行完所有任务，然后关闭
    }
}

void threadPool::addTask(std::function<void()> &&func)
{
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->tasks.emplace(std::forward<std::function<void()>>(func)); //完美转发，右值引用
    }
    pool_->cond.notify_one();
}