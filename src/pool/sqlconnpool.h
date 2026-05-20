#pragma once

/**
 * 连接池的概念无非就是，当连接请求的时候，就将池中的连接pop给使用者，然而当使用者使用完后，连接
 * 检查状态，满足条件则继续归还连接池 
 * 
 * 可以用RAII
 * 
 * 也可以直接使用unique_ptr自定义删除器
 */

#include <queue>
#include <mutex>
#include <mysql/mysql.h>
#include <condition_variable>
#include <assert.h>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <chrono>

using Seconds = std::chrono::seconds;


// 全局管理 SqlConnPool的生命周期
class MysqlLibrary {
public:
    MysqlLibrary() {
        mysql_library_init(0, nullptr, nullptr);
    }

    ~MysqlLibrary() {
        mysql_library_end();
    }
};


// class Mysql;

/**
 * @brief SqlConnPool 数据库连接池类
 * 
 * 该类实现了一个MySQL数据库连接池，用于管理和复用数据库连接，提高数据库访问效率。
 */
class SqlConnPool {
private:
    int max_count_;    // 连接池中最大连接数
    int use_count_;    // 当前正在使用的连接数
    int free_count_;   // 当前空闲的连接数
    bool is_stop = false; // 是否已经停止

    std::queue<MYSQL*> conns_;    // 存储MySQL连接的队列
    std::mutex mutex_;            // 互斥锁，用于保护连接池的线程安全
    std::condition_variable condition_;  // 条件变量，用于连接的获取和释放

    std::string host_;
    int port_;
    std::string user_;
    std::string pwd_;
    std::string dbName_;

    struct MysqlPoolDeleter {

        SqlConnPool* pool_ = nullptr;
        
        void operator()(MYSQL* ptr) {
            if (pool_)
                pool_->freeConn(ptr);
            else {
                if (ptr)
                    mysql_close(ptr);
            }
        }
    };

    void freeConn(MYSQL*);

    int getValidConnCount() const {
        return use_count_ + free_count_;
    }

public:
    SqlConnPool();     // 构造函数
    ~SqlConnPool();    // 析构函数

public:
    MYSQL* sql_connect() const;

    void init(const std::string& host, int port, const std::string& user,
        const std::string& pwd, const std::string& dbName, int connSize = 10);

    MYSQL* getConn();

    std::unique_ptr<MYSQL, MysqlPoolDeleter> getConnPlus();

    int getMaxCount() const {return max_count_;}
    int getFreeCount() const {return free_count_;}
    int getUseCount() const {return use_count_;}
};