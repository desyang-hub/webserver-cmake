/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#include "heaptimer.h"

#include <assert.h>

void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < nodes_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(nodes_[j] < nodes_[i]) { break; }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < nodes_.size());
    assert(j >= 0 && j < nodes_.size());
    std::swap(nodes_[i], nodes_[j]);
    index_[nodes_[i].id] = i;
    index_[nodes_[j].id] = j;
} 

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < nodes_.size());
    assert(n >= 0 && n <= nodes_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && nodes_[j + 1] < nodes_[j]) j++;
        if(nodes_[i] < nodes_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::addTimeNode(int id, int timeout, TimeoutCallBack cb) {
    assert(id >= 0);
    size_t i;
    if(index_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = nodes_.size();
        index_[id] = i;
        nodes_.push_back({id, Clock::now() + Ms(timeout), std::move(cb)});
        siftup_(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = index_[id];
        nodes_[i].expires = Clock::now() + Ms(timeout);
        nodes_[i].cb = std::move(cb);
        if(!siftdown_(i, nodes_.size())) {
            siftup_(i);
        }
    }
}

void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(nodes_.empty() || index_.count(id) == 0) {
        return;
    }
    size_t i = index_[id];
    TimerNode node = nodes_[i];
    node.cb();
    del_(i);
}

void HeapTimer::del_(int index) {
    /* 删除指定位置的结点 */
    assert(!nodes_.empty() && index >= 0 && index < nodes_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = nodes_.size() - 1;
    assert(i <= n);
    if(i < n) {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    index_.erase(nodes_.back().id);
    nodes_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!nodes_.empty() && index_.count(id) > 0);
    nodes_[index_[id]].expires = Clock::now() + Ms(timeout);;
    siftdown_(index_[id], nodes_.size());
}

void HeapTimer::tick() {
    /* 清除超时结点 */
    if(nodes_.empty()) {
        return;
    }
    while(!nodes_.empty()) {
        TimerNode node = nodes_.front();
        if(std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!nodes_.empty());
    del_(0);
}

void HeapTimer::clear() {
    index_.clear();
    nodes_.clear();
}

int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!nodes_.empty()) {
        res = std::chrono::duration_cast<Ms>(nodes_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}