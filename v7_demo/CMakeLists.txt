cmake_minimum_required(VERSION 3.10)
project(MyServerProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(server main.cpp Database.hpp Logger.hpp ThreadPool.hpp HttpRequest.hpp HttpResponse.hpp HttpServer.hpp Router.hpp FileUtils.hpp)



# 外部库链接（如果有其他库需求，可以在此处添加）
target_link_libraries(server PRIVATE sqlite3 pthread)


# 编译选项
# 如果需要，可以添加其他编译选项
# target_compile_options(server PRIVATE ...)

# 如果需要链接其他库，可以使用 find_package 或 find_library 查找并链接它们

