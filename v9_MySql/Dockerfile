#   指定特定版本的ubuntu镜像
FROM ubuntu:latest
#   替换为thu的ubuntu镜像源
RUN sed -i 's/http:\/\/ports.ubuntu.com/http:\/\/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list
#   更新软件包并安装所需要的库
RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential python3 python3-pip libsqlite3-dev nginx && \
    rm -rf /var/lib/apt/lists/*
#   配置Nginx   (假设已经创建了nginx.conf 并放在与 DockerFile 相同的目录下)
COPY nginx.conf /etc/nginx/nginx.conf
#   复制代码到容器中
COPY . /usr/src/myapp
#   设置工作目录
WORKDIR /usr/src/myapp
#   在DockerFile中创建缓存目录
RUN mkdir -p /var/cache/nginx
#   编译程序（确保您的编译命令适用于您的项目）
RUN g++ -o myserver main.cpp -sqlite3 -pthread
#   暴露端口 （Nginx和您的应用程序）
EXPOSE 80 8080 8081
#   创建并复制移动脚本到服务器
COPY start.sh /start.sh
RUN chmod +x /start.sh
#   启动脚本并运行 Nginx 和应用程序
CMD [ "/start.sh" ]