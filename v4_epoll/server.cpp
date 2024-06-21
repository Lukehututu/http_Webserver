#include <iostream>
#include <string>           //  引入字符串处理函数
#include <functional>       //  引入包装器
#include <map>              //  引入标准映射容器库
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>          //  istringstream
#include <sys/epoll.h>      //  用于引入epoll模型
#include <fcntl.h>          //  用于设置非阻塞模式


#include "Logger.h"         //  引入日志类头文件
#include "Database.h"       //  引入数据库类头文件
using namespace std;

#define PORT 8080           //  定义端口号
#define EPOLL_SZ 50         //  定义EPOLL监听的容量       
#define BUF_SZ 1024         //  指定缓冲区大小的宏       


void error_handling(string err);
void setNonBlocking(const int& fd);       //  设置成非阻塞模式   


using RequestHandler = function<string(const string&)>; //  定义请求处理函数类型

//  分别为GET和POST请求设置路由表
map<string,RequestHandler> get_routes;
map<string,RequestHandler> post_routes;

//  创建数据库的全局变量
Database db("user.db"); //  创建数据库对象


//  从请求体中解析用户名和密码
map<string,string> parseFromBody(const string& body) {
    map<string,string> params;
    istringstream stream(body);
    string pair;

    LOG_INFO("Parsing body: %s",body.c_str());

    while(getline(stream,pair,'&')) {
        string::size_type pos = pair.find('=');
        if (pos != string::npos) {
            string key = pair.substr(0,pos);
            string value = pair.substr(pos+1);
            params[key] = value;
            LOG_INFO("Parsed key-value pair: %s = %s",key.c_str(),value.c_str());
        } else {
            //  错误处理 找不到"="分隔符
            string error_msg = "Error parsing: "+pair;
            LOG_ERROR(error_msg.c_str());   //  记录错误信息
            cerr << error_msg <<endl;
        }
    }
    return params;
}

//  初始化路由表    --  为不同的路径设置不同的处理函数  --主要作用是填充，因此返回值是void
void setupRoutes(){
    LOG_INFO("Setting up routes");                  //记录路由设置
    //  GET请求处理
    get_routes["/"] = [](const string body){
        return "Hello World!";
    };
    get_routes["/register"] = [](const string body){
        //  实现用户注册逻辑
        return "Please use post to register";
    };
    get_routes["/login"] = [](const string body){
        //  实现用户登录逻辑
        return "Please use post to login";
    };
    
    //  POST请求处理
    post_routes["/register"] = [](const string body){
        //  解析用户名和密码
        //  例如从请求中解析 username 和 password 这里需要自己实现解析逻辑
        auto params = parseFromBody(body);
        string username = params["username"];
        string password = params["password"];
        //  调用数据库的方法进行注册
        if (db.registerUser(username,password))
            return "Register success";
        else
            return "Register failed";
    };
    //  修改登录路由
    post_routes["/login"] = [](const string body){
        //  解析用户名和密码
        //  例如从请求中解析 username 和 password 这里需要自己实现解析逻辑
        auto params = parseFromBody(body);
        string username = params["username"];
        string password = params["password"];
        LOG_INFO("parse body success,username: %s,password: %s",username.c_str(),password.c_str());
        if (db.loginUser(username,password))
            return "Login success";
        else
            return "Login fail";
    };
}


//  解析http请求
tuple<string,string,string> parsehttpRequest(const string request){
    //  找到第一个空格确定请求方法
    size_t method_end = request.find(" ");
    //  提取方法
    string method = request.substr(0,method_end);
    //  找到第二个空格确定uri
    size_t url_end = request.find(" ",method_end+1);
    //  提取uri
    string url = request.substr(method_end+1,url_end - method_end - 1);
    //  提取请求体，对于POST来说
    string tmp(request);
    size_t body_start = tmp.find("\r\n\r\n");
    string body = tmp.substr(body_start + 4);
    //  返回提取出的方法和uri
    return {method,url,body};
}

//  处理http请求    --  需要请求方法（method）、请求路径（uri）、请求体（body）
string handlehttpRequest(const string method,const string url,const string body){
    LOG_INFO("Handling HTTP request for URL: %s",url.c_str());
    //  检查是否是GET请求
    if(method == "GET" && get_routes.count(url) > 0) {
        return get_routes[url](body);
    }else if(method == "POST" && post_routes.count(url) > 0) {
        return post_routes[url](body);
    }else {
        return "404 Not Found";
    }
}


int main(int argc,char* argv[]){

    if(argc != 2){
        perror("argc");
    }

    //  1.创建相关变量
    string method,uri;
    int listen_fd;
    int clnt_fd;
    struct sockaddr_in listen_addr,clnt_addr;
    socklen_t socklen = sizeof(clnt_addr);

    //  2.绑定变量
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd == -1) {
        error_handling("socket()");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    setNonBlocking(listen_fd);
    LOG_INFO("socket created");

    //  填充地址结构体
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(argv[1]));
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //  3.bind
    if(bind(listen_fd,(struct sockaddr*)&listen_addr,sizeof(listen_addr)) == -1){
        error_handling("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    //  4.listen
    if(listen(listen_fd,5) == -1){
        error_handling("listen");
        close(listen_fd);
    }
    LOG_INFO("Server listening on port %d",PORT);   //记录服务器监听
    
    //  设置路由
    setupRoutes();
    LOG_INFO("Server starting");

    //  创建epoll实例
    int ep_cnt = 0;
    int epfd = epoll_create(EPOLL_SZ);
    if( epfd == -1) {
        LOG_ERROR("epoll_create -1");  //  注册时间失败，记录错误日志
        exit(EXIT_FAILURE);
    }
    //  给events分配空间
    size_t ep_sz = sizeof(struct epoll_event);
    auto events = new struct epoll_event[EPOLL_SZ * ep_sz];

    //  先将主线程的listen_fd以读事件注册进去
    struct epoll_event event;       //用来存储具体变量信息
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&event) == -1) {
        LOG_ERROR("epoll_ctl: listen_fd");  //  注册事件失败，记录错误日志
        exit(EXIT_FAILURE);
    }
    

    //  处理请求的循环体
    char buffer[BUF_SZ];
    ssize_t strlen = 0;
    while(1){
        //  开始监听
        ep_cnt = epoll_wait(epfd,events,EPOLL_SZ,-1);
        if (ep_cnt == -1) {
            LOG_ERROR("epoll_wait error");
            break;
        }
        //  此时检测到有要读的信息，开始轮询(事件轮询)
        for(int i = 0;i < ep_cnt ;++i) {
            //  此时说明有新的连接，那就处理新的连接
            if(events[i].data.fd == listen_fd) {
                clnt_fd = accept(listen_fd,(struct sockaddr*)&clnt_addr,&socklen);
                setNonBlocking(clnt_fd);    //  设置新连接为非阻塞模式
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clnt_fd;
                if (epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_fd,&event) == -1) {
                    LOG_ERROR("Error adding new socket to epoll");
                    exit(EXIT_FAILURE);
                } else {
                    LOG_INFO("New connection accepted");
                }
            }
            //  此时就是普通客户端来的请求
            else {
                //  边缘触发就靠重复读取该事件的缓冲区来获得所有数据
                string Request;
                while(1) {
                    strlen = read(events[i].data.fd,buffer,BUF_SZ);
                    if( strlen == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            //  没有数据可读了
                            break;
                        } else {
                            //  此时真正的出错了
                            error_handling("read");
                            close(events[i].data.fd);
                            break;
                        }
                    } else if ( strlen == 0) {
                        break;
                    } else {
                        buffer[strlen] = '\0';
                        Request += buffer;
                    }
                }
                auto [method,url,body] = parsehttpRequest(Request);
                auto response_body = handlehttpRequest(method,url,body);
                string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
                send(events[i].data.fd,response.c_str(),response.size(),0);
                //  将其移除epoll池
                epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,nullptr);
                close(events[i].data.fd);
                LOG_INFO("Request handled and response sent on socket: %d",events[i].data.fd);
            }

        }   
    }
    //  7.close socket
    close(listen_fd);
    close(epfd);
    delete[] events;
}

void error_handling(string err){
    perror(err.c_str());
}

//  将目标套接字设置为非阻塞模式
void setNonBlocking(const int& fd) {

    /* fcntl(int fd,        --  要操作目标的fd
             int cmd,       --  表示函数调用的目的
             ...        )*/
    //  获取之前设置的属性信息  -- 如果cmd传F_GETFL,那返回第一个参数所指的fd的属性
    int flag = fcntl(fd,F_GETFL);
    //  修改属性 -- 在此基础上添加非阻塞标志O_NONBLOCK 利用|添加
    flag |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flag);
    //  这样当调用recv&send时都会形成非阻塞文件
}
