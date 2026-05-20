#include <iostream>
#include <mysql/mysql.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>
#include "pool/threadpool.h"
#include "pool/sqlconnpool.h"

using Seconds = std::chrono::seconds;

using namespace std;
using namespace muduo;

std::string m_url;
int m_Port;
std::string m_User;
std::string m_PassWord;
std::string m_DatabaseName;
int m_close_log;

void init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;
	m_close_log = close_log;

    MYSQL *con = NULL;
    con = mysql_init(con);

    if (con == NULL)
    {
        LOG_ERROR << "MySQL Error";
        exit(1);
    }
    auto conn = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

    if (conn == NULL)
    {
        LOG_ERROR << "MySQL Error";
        LOG_ERROR << mysql_error(con);
        exit(1);
    }

    // 执行查询
    int d = mysql_query(conn, "select * from user;");

    auto res = mysql_store_result(conn);

    LOG_INFO << res;

    int row = mysql_num_rows(res);

    LOG_INFO << "num row: " << row;
}

int worker(int id) {
    std:this_thread::sleep_for(Seconds(1));
    // std::cout << "thread_id: " << std::this_thread::get_id() << std::endl;
    // std::cout << "worker id: " << id << std::endl;

    return id;
}


int main() {

    // init("127.0.0.1", "admin", "admin", "webserver", 3306, 2, 1);

    // ThreadPool pool(4);

    // std::vector<std::future<int>> res;

    // for (int i = 0; i < 4; ++i) {
    //     res.push_back(pool.enqueue(worker, i));
    // }

    // for (int i = 0; i < 4; ++i) {
    //     std::cout << res[i].get() << std::endl;
    // }

    SqlConnPool pool;
    pool.init("127.0.0.1", 3306, "admin", "admin", "webserver", 2);

    auto ptr = pool.getConnPlus();

    auto conn = ptr.get();

    // 执行查询
    int d = mysql_query(conn, "select * from user;");

    auto res = mysql_store_result(conn);

    LOG_INFO << res;

    int row = mysql_num_rows(res);

    LOG_INFO << "num row: " << row;

    std::cout << pool.getFreeCount() << std::endl;

    ptr.reset();
    std::cout << pool.getFreeCount() << std::endl;


    return 0;
}