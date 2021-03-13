#include "timer.h"

void Timer::add(int id, int timeOut, const timeoutCallBack &cb)
{
    assert(id >= 0);
    size_t idx;
    //如果没有，就添加
    if (ref_.count(id) == 0)
    {
        idx = heap_.size();
        //索引一下
        ref_[id] = idx;
        heap_.push_back({id, Clock::now() + Ms(timeOut), cb});
        siftUp_(idx);
    }
    else
    {
        idx = ref_[id];
        heap_[idx].expires = Clock::now() + Ms(timeOut);
        heap_[idx].cb = cb;
        if (!siftUp_(idx))
            siftDown_(idx, heap_.size());
    }
}

void Timer::del_(int idx)
{
    assert(!heap_.empty() && idx < heap_.size() - 1 && idx >= 0);
    size_t lastIdx = heap_.size() - 1;

    swapNode_(idx, lastIdx);
    if (!siftUp_(idx))
        siftDown_(idx, heap_.size());
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}
bool Timer::siftUp_(size_t idx)
{
    size_t n = heap_.size();
    assert(idx >= 0 && idx < n);
    size_t tempIdx = idx;
    int parentIdx = (idx - 1) >> 1;
    while (parentIdx >= 0)
    {
        if (heap_[parentIdx] < heap_[idx])
            break;
        swapNode_(parentIdx, idx);
        idx = parentIdx;
        parentIdx = (idx - 1) >> 1;
    }
    return idx != tempIdx;
}

void Timer::adjust(int id, int timeOut)
{
    assert(!heap_.empty() && ref_.count(id) > 0);
    size_t idx = ref_[id];
    heap_[idx].expires = Clock::now() + Ms(timeOut);
    if (!siftUp_(idx))
        siftDown_(idx, heap_.size() - 1);
}

void Timer::swapNode_(size_t idx1, size_t idx2)
{
    assert(idx1 >= 0 && idx1 < heap_.size() && idx2 >= 0 && idx2 < heap_.size());
    std::swap(heap_[idx1], heap_[idx2]);
    ref_[heap_[idx1].id] = idx2;
    ref_[heap_[idx2].id] = idx1;
}
//这个n就是堆大小
bool Timer::siftDown_(size_t idx, size_t n)
{
    assert(idx < heap_.size() && idx >= 0);
    int idxTemp = idx;
    int ChildIdx = idx * 2 + 1; //左孩子
    while (ChildIdx < n)
    {
        if (ChildIdx + 1 < n && heap_[ChildIdx + 1] < heap_[ChildIdx])
            ChildIdx++;
        if (heap_[idx] < heap_[ChildIdx])
            break;
        swapNode_(idx, ChildIdx);
        idx = ChildIdx;
        ChildIdx = idx * 2 + 1;
    }

    return idxTemp != idx;
}
void Timer::pop()
{
    assert(!heap_.empty());
    del_(0);
}

void Timer::clear()
{
    ref_.clear();
    heap_.clear();
}
//tick一次，清理一下过期时间
void Timer::tick()
{
    if (!heap_.empty())
        return;
    while (!heap_.empty())
    {
        auto node = heap_.front();
        if (std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0)
            break;
        //超时关闭当前这个id的http连接
        node.cb();
        pop();
    }
}

int Timer::getNextTick()
{
    tick();
    size_t res = -1;
    if (!heap_.empty())
    {
        res = std::chrono::duration_cast<std::chrono::milliseconds>(heap_.front().expires - Clock::now()).count();
        if (res < 0) //返回过期时间，如果有刚过期的，那就重新获取下一个
            res = 0;
    }
    return res;
}