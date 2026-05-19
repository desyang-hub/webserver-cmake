#pragma once

#include <iostream>
#include "toml++/toml.hpp"

enum Mode {
    LT,
    ET
};

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

const std::string SERVER_SCOPE = "server";
const std::string DATABASE_SCOPE = "database";
const std::string DEFAULT_SCOPE = "params";

struct Config
{
    // 服务设置
    std::string ip;
    int port;
    int mode;
    int timeoutMs;
    bool is_safe_quit;

    // 数据库设置
    std::string mysql_host;
    int mysql_port;
    std::string mysql_username;
    std::string mysql_password;
    std::string mysql_database_name;

    // 连接池设置
    int conn_pool_size;
    int thread_pool_size;
    bool is_logging_open;
    int log_level;
    int async_loggin_queue_size;

    explicit Config(const std::string& config_path = "config.toml") {
        auto config = toml::parse_file( config_path );

        // [server]
        ip = config[SERVER_SCOPE]["ip"].value_or("127.0.0.1");
        port = config[SERVER_SCOPE]["port"].value_or(8080);
        mode = config[SERVER_SCOPE]["mode"].value_or(ET);
        timeoutMs = config[SERVER_SCOPE]["timeout_ms"].value_or(60000);
        is_safe_quit = config[SERVER_SCOPE]["is_safe_quit"].value_or(true);

        // [database]
        mysql_host = config[DATABASE_SCOPE]["mysql_host"].value_or("127.0.0.1");
        mysql_port = config[DATABASE_SCOPE]["mysql_port"].value_or(3306);
        mysql_username = config[DATABASE_SCOPE]["mysql_username"].value_or("root");
        mysql_password = config[DATABASE_SCOPE]["mysql_password"].value_or("root");

        mysql_database_name = config[DATABASE_SCOPE]["mysql_database_name"].value_or("webserver");
    }

    static const Config& LoadSystemConfig() {
        static const Config config_;
        return config_;
    }

    friend std::ostream& operator<<(std::ostream& os, const Config& config) {
        os << "mysql_username: " << config.mysql_username << std::endl;
        os << "mysql_password: " << config.mysql_password;

        return os;
    }
};


// 1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
// 3306, "admin", "admin", "webserver", /* Mysql配置 */
// 12, 6, true, 1, 1024); /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */