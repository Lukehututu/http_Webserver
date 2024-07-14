//  该文件--服务器的框架
#pragma once

#include <arpa/inet.h>     
#include <sys/socket.h>          
#include <sys/epoll.h>      //  引入epoll
#include <fcntl.h>          //  进行非阻塞模式设置
#include <unistd.h>         //  IO函数

#include "Database.hpp"     //  引入数据库
#include "Logger.hpp"       //  引入日志
#include "ThreadPool.hpp"   //  引入线程池
#include "Router.hpp"       //  引入路由
#include "HttpResponse.hpp" //  引入响应
#include "FileUtils.hpp" //  引入响应

#define PORT 8080           //  定义端口
#define EPOLL_SIZE 50       //  定义监听最大的数量
#define BUF_SIZE 4096       //  定义缓冲区大小


class HttpServer {
public:
    //  构造函数，初始化成员变量并传入参数（端口号，epoll的最大监听事件，以及数据库的使用）
    HttpServer(int port,int max_events,Database& db)
    :port(port),max_events(max_events),db(db),server_fd(-1),epoll_fd(-1){}

    //  启动服务器，设置套接字，epoll，启动线程池并进入主循环
    void start() {
        setupServerSocket();    //  创建并配置服务器套接字
        setupEpoll();           //  创建epoll实例
        ThreadPool pool(4);    //  创建线程池

        //  初始化epoll_event数组
        auto events = new epoll_event[max_events];
        LOG_INFO("start loop");
        //  主循环
        while(1) {
            LOG_INFO("1");
            int nfds = epoll_wait(epoll_fd,events,max_events,-1);   //  开始监听
            LOG_INFO("2");
            //  轮询事件
            for(int i = 0;i < nfds;++i) {
                if(events[i].data.fd == this->server_fd) {
                    acceptConnection();
                } else {
                    int fd = events[i].data.fd;
                    pool.enqueue([fd,this]{
                        this->handleConnection(fd);
                    });
                }
            }
        }
        delete[] events;
    }
      
    //  析构，释放资源关闭连接
    ~HttpServer() {
        close(server_fd);
        close(epoll_fd);
    }   


private:

    int server_fd;  //  服务器的监听socket
    int port;       //  服务器使用的端口
    int max_events; //  能够监听的最多的端口数
    int epoll_fd;   //  epoll实例的文件描述符
       //  epoll实例的文件描述符

    Database& db;    //  数据库    

    Router router;  //  路由器处理路由分发

    //  初始化路由
    void setupRoutes() {
        //  添加根路由处理器，返回"Hello World"响应
        this->router.addRoute("GET","/",[this](const HttpRequest& req){ 
            HttpResponse response;
            response.setBody("Hello World!");
            response.setHeader("Content-Type","text/plain");
            response.setStatusCode(200);
            return response;
        });
        //  添加索引界面的路由
        this->router.addRoute("GET","/index.html",[this](const HttpRequest& req){ 
            HttpResponse response;
            response.setBody(FileUtils::readfile("../static_rc/index.html"));
            response.setHeader("Content-Type","text/html");
            response.setStatusCode(200);
            return response;
        });
        
        //  设置与数据库有关的路由
        router.setupDatabaseRoutes(this->db);
        //  其他路由在这里添加...
        //  GET登录和注册 -- 获取静态资源           但实际上对于get请求的处理函数不传request也没事，因为没用到
        this->router.addRoute("GET","/login",[this](HttpRequest request) {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type","text/html");
            response.setBody(FileUtils::readfile("../static_rc/login.html")); //  读取html文件
            return response;
        });
        this->router.addRoute("GET","/register",[this](HttpRequest request) {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type","text/html");
            response.setBody(FileUtils::readfile("../static_rc/register.html")); //  读取html文件
            return response;
        });
    }

     //  初始化服务器
    void setupServerSocket() {
        
        struct sockaddr_in serveraddr;
        socklen_t socklen = sizeof(struct sockaddr_in);
        
        this->server_fd = socket(AF_INET,SOCK_STREAM,0);
        if(this->server_fd == -1) {
            LOG_INFO("socket failed");
            exit(EXIT_FAILURE);
        } 
        setNonBlocking(this->server_fd);
        LOG_INFO("socket created");

        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(server_fd,(struct sockaddr*)&serveraddr,socklen) == -1) {
            LOG_INFO("bind failed");
            exit(EXIT_FAILURE);
        }

        if(listen(server_fd,5) == -1) {
            LOG_INFO("listen failed");
            exit(EXIT_FAILURE);
        }
        LOG_INFO("Server listening on port %d",port);   //记录服务器监听

        //  初始化路由
        this->setupRoutes();

    }

    
    //  创建epoll实例
    void setupEpoll() {
        epoll_fd = epoll_create(EPOLL_SIZE);
        //  将监听socket先注册进去
        struct epoll_event event;
        event.data.fd = this->server_fd;
        event.events = EPOLLIN | EPOLLET;
        if(epoll_ctl(this->epoll_fd,EPOLL_CTL_ADD,this->server_fd,&event) == -1) {
            LOG_ERROR("epoll_ctl: server_fd");
            exit(EXIT_FAILURE);
        }
    }

    //  接收新的连接
    void acceptConnection() {
        LOG_INFO("new connection");
        struct sockaddr_in clntaddr;
        socklen_t socklen = sizeof(clntaddr);
        int clnt_fd;
        while((clnt_fd = accept(this->server_fd,(struct sockaddr*)&clntaddr,&socklen)) > 0 ) {
            setNonBlocking(clnt_fd);
            //  将其注册到epoll_events中
            struct epoll_event event;
            event.data.fd = clnt_fd;
            event.events = EPOLLIN | EPOLLET;
            if(epoll_ctl(this->epoll_fd,EPOLL_CTL_ADD,clnt_fd,&event) == -1) {
                LOG_ERROR("Error adding new socket on epoll");
                close(clnt_fd);
                exit(EXIT_FAILURE);
            } else {
                LOG_INFO("New connection accepted");
            }
        }

        //  如果accept失败且错误原因不是EAGAIN或EWOULDBLOCK，那就说明有错误，则报错
        if(clnt_fd == -1 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
            LOG_ERROR("Error adding new socket on epoll");
        }
    }

    //  处理新的事件
    void handleConnection(int fd) {
        LOG_INFO("handle");
        char buf[BUF_SIZE];
        string tmp;
        HttpRequest request;
        while(1) {
            ssize_t strlen = read(fd,buf,BUF_SIZE - 1);
            if(strlen == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    //  此时就是没数据可读了
                    break;
                } else {
                    //  此时才是真正出错了
                    LOG_INFO("read error");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            } else if(strlen == 0) {
                LOG_INFO("disConnecting");
                break;
            } else {
                buf[strlen] = '\0';
                tmp += buf;
            }  
        }
        //  此时读完了输入缓冲区的数据
        //  解析Request数据并通过Rouer获得HttpResponse对象
        if(request.parse(tmp)) {
            HttpResponse  response = router.routeRequest(request);
            string response_str = response.toString();
            send(fd,response_str.c_str(),response_str.size(),0);
            string headers;
            int pos = response_str.find("\r\n\r\n");
            headers = response_str.substr(0,pos);
            LOG_INFO(headers.c_str());
        }

        //  关闭socket
        close(fd);
    }


    //  设置为非阻塞模式
    void setNonBlocking(int fd) {

        int flag = fcntl(fd,F_GETFL);
        flag |= O_NONBLOCK;
        fcntl(fd,F_SETFL,flag);
    }


};