#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>

int main() {
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;
    sql::ResultSet *res;

    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "admin", "admin");
    con->setSchema("webserver");

    stmt = con->createStatement();
    res = stmt->executeQuery("SELECT username FROM user");

    while (res->next()) {
        std::cout << "User: " << res->getString("username") << std::endl;
    }

    delete res;
    delete stmt;
    delete con;


    return 0;
}

