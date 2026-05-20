// test.cpp
#include "buffer/buffer.h"
#include <assert.h>
#include <string>

int main() {
    Buffer buf(8);

    // 测试 append + retrieve
    buf.append("12345");
    assert(buf.readableBytes() == 5);
    assert(buf.writableBytes() == 3);

    buf.append("67"); // total 7 bytes
    assert(buf.readableBytes() == 7);
    assert(buf.writableBytes() == 1);

    buf.append("8"); // fill up
    assert(buf.writableBytes() == 0);

    // 触发 copyToFront + 扩容
    buf.append("9"); 
    assert(buf.readableBytes() == 9);
    assert(buf.prependableBytes() == 0); // 因为 copyToFront 后 readable 从 0 开始

    // 测试 retrieve
    auto s1 = buf.retrieveAsString(5);
    assert(s1 == "12345");
    assert(buf.readableBytes() == 4);

    auto s2 = buf.retrieveAllAsString();
    assert(s2 == "6789");
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == buf.store_.size()); // 全部可写

    // 测试空 append
    buf.append("");
    assert(buf.readableBytes() == 0);

    return 0;
}