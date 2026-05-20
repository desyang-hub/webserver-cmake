#pragma once

#include <vector>
#include <stddef.h>
#include <iostream>
#include <assert.h>

// |prepareable|readable|writeable| 缓冲区样式
// 当空间充足时，在readable处写入数据

const size_t DEFAULT_STORE_SIZE = 1024;

class Buffer {
public:
    std::vector<char> store_;

private:
    size_t readerIndex_;   // 写入的数据起始点
    size_t writerIndex_;  // 写入数据的结束点

private:
    void copyToBack(const std::string& msg) {
        std::copy(msg.begin(), msg.end(), store_.data() + writerIndex_);
        writerIndex_ += msg.size();
    }

    void copyToFront() {
        std::copy(store_.data() + readerIndex_, store_.data() + writerIndex_, store_.begin());
        writerIndex_ = readableBytes();
        readerIndex_ = 0;
    }
    
public:
    Buffer(size_t default_store_size=DEFAULT_STORE_SIZE) : readerIndex_(0), writerIndex_(0) {
        store_.resize(default_store_size);
    }

public:
    // 将prepareable_和readable光标向前移动
    void retrieve(size_t len) {
        assert(readerIndex_ + len <= writerIndex_);
        readerIndex_ += len;
    }

    // 移动并将长度为len的字符串返回
    std::string retrieveAsString(size_t len) {
        std::string bytes(store_.data() + readerIndex_, len);
        retrieve(len);
        return bytes;
    }

    void retrieveAll() {
        readerIndex_ = writerIndex_;
    }

    std::string retrieveAllAsString() {
        std::string bytes = retrieveAsString(readableBytes());
        retrieveAll();
        return bytes;
    }

    // 将数据
    void append(const std::string& msg) {
        if (msg.empty()) return;

        // 1. 剩余空间足够写入
        if (writerIndex_ + msg.size() <= store_.size()) {
            copyToBack(msg);
        } else if(msg.size() <= writableBytes()) { // 2. 空间足够，需要前移prepareable
            copyToFront();
            copyToBack(msg);
        } else { // 3. 空间不足，前移后，先扩容继续将后半部分数据写入
            copyToFront();
            // 扩容
            store_.resize(readableBytes() + msg.size());
            copyToBack(msg);
        }
    }

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const {
        return store_.size() - readableBytes();
    }

    size_t prependableBytes() const {
        return readerIndex_;
    }
};