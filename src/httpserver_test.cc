#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/EventLoop.h>

using namespace muduo::net;

void onRequest(const HttpRequest& req, HttpResponse* resp) {
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Connection", "close");
    resp->setBody("Hello, world!\n");
}

int main() {
    EventLoop loop;
    InetAddress listenAddr(8080);
    HttpServer server(&loop, listenAddr, "example");

    server.setHttpCallback(onRequest); // 设置 HTTP 请求回调
    server.start();
    loop.loop();
}