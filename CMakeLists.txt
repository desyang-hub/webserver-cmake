cmake_minimum_required(VERSION 3.10)

project(webserver-cmake C CXX)

add_executable(main
    src/main.cc
)

target_link_libraries(main
    PRIVATE
    muduo_base
    mysqlclient
)

add_executable(db 
    src/db.cc
)

target_link_libraries(db
    PRIVATE
    muduo_base
    mysqlcppconn
)

add_executable(config
    src/config/config_test.cc
)

target_include_directories(config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# 将 config.toml 从源码目录复制到构建目录（保持同名）
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config/config.toml   # 源文件
    ${CMAKE_CURRENT_BINARY_DIR}/config.toml   # 目标文件
    COPYONLY                         # 关键！表示仅复制，不处理变量替换
)