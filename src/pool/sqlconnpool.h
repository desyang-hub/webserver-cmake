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

    struct MysqlPoolDeleter {

        SqlConnPool* pool_ = nullptr;
        
        void operator()(MYSQL* ptr) {
            if (ptr && pool_) {
                pool_->freeConn(ptr);
            } else {
                if (ptr)
                    mysql_close(ptr);
            }
        }
    };

public:
    SqlConnPool();     // 构造函数
    ~SqlConnPool();    // 析构函数

public:
    void sql_connect(const char* host, int port,
        const char* user,const char* pwd, const char* dbName);

    void init(const char* host, int port,
        const char* user,const char* pwd, const char* dbName,
        int connSize = 10);

    MYSQL* getConn();

    std::unique_ptr<MYSQL, MysqlPoolDeleter> getConnPlus();

    void freeConn(MYSQL*);

    int getMaxCount() const {return max_count_;}
    int getFreeCount() const {return free_count_;}
    int getUseCount() const {return use_count_;}
};

inline SqlConnPool::SqlConnPool() : use_count_(0), free_count_(0) {
}

inline SqlConnPool::~SqlConnPool() {
    std::lock_guard<std::mutex> lock(mutex_);

    while (!conns_.empty()) {
        MYSQL* conn = conns_.front();
        conns_.pop();
        mysql_close(conn); // 回收sql连接
    }

    mysql_library_end();
}

void SqlConnPool::sql_connect(const char* host, int port,
    const char* user,const char* pwd, const char* dbName) {
        MYSQL *sql = nullptr;
        auto conn = mysql_init(sql);
        
        assert(conn);
        
        sql = mysql_real_connect(conn, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);

        assert(sql);

        // if (!sql) {
        //     // 获取错误信息和错误码
        //     const char* errorMsg = mysql_error(conn);   // 注意：即使 sql 为 nullptr，mysql_error 仍可工作（见下方说明）
        //     unsigned int errorCode = mysql_errno(conn);

        //     LOG_ERROR("MySQL Connect failed! Error %d: %s", errorCode, errorMsg);

        //     printf("MySQL Connect failed! Error %d: %s", errorCode, errorMsg);

        //     // LOG_ERROR("MySql Connect error!");
        // }
        ++free_count_;
        conns_.push(sql);
    }


// 初始化连接池
inline void SqlConnPool::init(const char* host, int port,
    const char* user,const char* pwd, const char* dbName,
    int connSize) {
        assert(connSize > 0);

        max_count_ = connSize;

        for (int i = 0; i < connSize; ++i) {
            sql_connect(host, port, user, pwd, dbName);
        }
    }

// 从连接池中申请一个连接, failed return nullptr
MYSQL* SqlConnPool::getConn() {
    MYSQL* sql{};
    {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this]{
            return !conns_.empty() || is_stop;
        });

        if (!conns_.empty()) {
            sql = std::move(conns_.front());
            conns_.pop();


            --free_count_;
            ++use_count_;
        }
    }
    return sql;
}


std::unique_ptr<MYSQL, SqlConnPool::MysqlPoolDeleter> SqlConnPool::getConnPlus() {
    MYSQL* sql{};
    {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this]{
            return !conns_.empty() || is_stop;
        });

        if (!conns_.empty()) {
            sql = std::move(conns_.front());
            conns_.pop();

            --free_count_;
            ++use_count_;
        }
    }
    return std::unique_ptr<MYSQL, MysqlPoolDeleter>(sql, {this});
}

void SqlConnPool::freeConn(MYSQL* sql) {
    assert(sql);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns_.emplace(sql);

        ++free_count_;
        --use_count_;
    }
    condition_.notify_one();
}