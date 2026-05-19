#pragma once

#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>

const int HEAP_TIMER_INIT_SIZE = 64;

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using Ms = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;


struct TimerNode {
    int id;
    TimeStamp expires; // 过期时间
    TimeoutCallBack cb;

    bool operator<(const TimerNode& timeNode) {
        return expires < timeNode.expires;
    }
};

class HeapTimer {
public:
    HeapTimer() {
        // 预留大空间，避免频繁扩容
        nodes_.reserve(HEAP_TIMER_INIT_SIZE);
    }

    ~HeapTimer() {
        clear();
    }

public:
    // 清空计时器
    void clear();

    // 调整计时器id的计时时间
    void adjust(int id, int timeout);

    // 添加计时器
    void addTimeNode(int id, int timeout, TimeoutCallBack cb);

    // 调用计时器id的回调函数
    void doWork(int id);

    // 计时
    void tick();

    // 弹出计时器
    void pop();

    int getNextTick();

private:
    // 删除id编号计时器
    void del_(int id);

    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void SwapNode_(size_t i, size_t j);


private:
    std::vector<TimerNode> nodes_;

    std::unordered_map<int, int> index_; // 存放索引映射
};