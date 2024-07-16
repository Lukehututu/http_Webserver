cmake_minimum_required(VERSION 3.10)
project(MyServerProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置app输出路径
set(HOME /home/luk1/code/http_WebServer/v8_MySql)   # 定义home路径
set(EXECUTABLE_OUTPUT_PATH ${HOME}/app) # 指定输出路径

#   指定配置文件
set(NGINX_CONF "nginx.conf")
#   安装配置文件到目标位置
install(FILES ${NGINX_CONF}
        DESTINATION /etc/nginx
        COMPONENT config_files)

# 添加可执行文件
add_executable(server main.cpp Database.hpp Logger.hpp ThreadPool.hpp HttpRequest.hpp HttpResponse.hpp HttpServer.hpp Router.hpp FileUtils.hpp)

# 手动添加目录和库的位置
include_directories(/usr/include/mysql)
target_link_libraries(server PRIVATE -L/usr/lib64/mysql -lmysqlclient pthread)

# 外部库链接（如果有其他库需求，可以在此处添加）
#target_link_libraries(server PRIVATE mysql pthread)


# 编译选项
# 如果需要，可以添加其他编译选项
# target_compile_options(server PRIVATE ...)

# 如果需要链接其他库，可以使用 find_package 或 find_library 查找并链接它们

