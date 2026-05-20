#include "sqlconnpool.h"

// 用于在程序开始和结束过程分别执行 mysql_library_init 和 mysql_library_end方法
static MysqlLibrary g_mysql_library;


SqlConnPool::SqlConnPool() : use_count_(0), free_count_(0) {}

SqlConnPool::~SqlConnPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        is_stop = true;
    }
    condition_.notify_all();

    std::lock_guard<std::mutex> lock(mutex_);

    while (!conns_.empty()) {
        MYSQL* conn = conns_.front();
        conns_.pop();
        mysql_close(conn); // 回收sql连接
    }
}

MYSQL* SqlConnPool::sql_connect() const {
        auto conn = mysql_init(nullptr);
        if (!conn) {
            throw std::runtime_error("mysql init error");
        }
        
        MYSQL *sql = mysql_real_connect(conn, host_.c_str(), user_.c_str(), pwd_.c_str(),
                                 dbName_.c_str(), port_, nullptr, 0);

        if (!sql) {
            mysql_close(conn);

            throw std::runtime_error("Failed to connect to MySQL: " + std::string(mysql_error(conn)));
        }
        
        return sql;
    }


// 初始化连接池
void SqlConnPool::init(const std::string& host, int port, const std::string& user,
    const std::string& pwd, const std::string& dbName, int connSize = 10) {
        assert(connSize > 0);

        host_ = host;
        port_ = port;
        user_ = user;
        pwd_ = pwd;
        dbName_ = dbName;

        for (int i = 0; i < connSize; ++i) {
            MYSQL* sql_conn = sql_connect();
            conns_.emplace(sql_conn);
            free_count_++;
        }
    }

// 从连接池中申请一个连接, failed return nullptr
MYSQL* SqlConnPool::getConn() {
    MYSQL* sql{};
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // 超时返回nullptr;
        if (!condition_.wait_for(lock, Seconds(5), [this] {
            return !conns_.empty() || is_stop;
        })) {
            return nullptr;
        }

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
    MYSQL* sql = getConn();

    if (!sql) {
        throw std::runtime_error("Get MYSQL* nullptr.");
    }

    return std::unique_ptr<MYSQL, MysqlPoolDeleter>(sql, {this});
}

void SqlConnPool::freeConn(MYSQL* sql) {
    if (!sql) {
        return;
    }

    bool need_reconnect = (mysql_ping(sql) != 0);

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_stop) {
            mysql_close(sql);
            use_count_--;
            return;
        }
    
        if (need_reconnect) {
            mysql_close(sql);
            sql = nullptr;
            try
            {
                sql = sql_connect();
            }
            catch(const std::exception& e)
            {
                std::cerr << "reconnect failed: " << e.what() << std::endl;
            }
        }
        
        if (sql) {
            conns_.emplace(sql);
            ++free_count_;
        }

        --use_count_;
    }

    condition_.notify_one();
}