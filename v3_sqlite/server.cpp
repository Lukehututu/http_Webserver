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


#include "Logger.h"         //  引入日志类头文件
#include "Database.h"       //  引入数据库类头文件
using namespace std;

#define PORT 8080           //定义端口号
void error_handling(string err);

using RequestHandler = function<string(const string&)>; //  定义请求处理函数类型

//  分别为GET和POST请求设置路由表
map<string,RequestHandler> get_routes;
map<string,RequestHandler> post_routes;

//  创建数据库的全局变量
Database db("user.db"); //  创建数据库对象


//  从请求体中解析用户名和密码
map<string,string> parseFromBody(const string& request) {
    //  从请求中解析出请求体
    string tmp(request);
    size_t body_start = tmp.find("\r\n\r\n");
    string body = tmp.substr(body_start + 4);

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
    get_routes["/"] = [](const string request){
        return "Hello World!";
    };
    get_routes["/register"] = [](const string request){
        //  实现用户注册逻辑
        return "Please use post to register";
    };
    get_routes["/login"] = [](const string request){
        //  实现用户登录逻辑
        return "Please use post to login";
    };
    
    //  POST请求处理
    post_routes["/register"] = [](const string request){
        //  解析用户名和密码
        //  例如从请求中解析 username 和 password 这里需要自己实现解析逻辑
        auto params = parseFromBody(request);
        string username = params["username"];
        string password = params["password"];

        //  调用数据库的方法进行注册
        if (db.registerUser(username,password))
            return "Register success";
        else
            return "Register failed";
    };
    //  修改登录路由
    post_routes["/login"] = [](const string request){
        //  解析用户名和密码
        //  例如从请求中解析 username 和 password 这里需要自己实现解析逻辑
        auto params = parseFromBody(request);
        string username = params["username"];
        string password = params["password"];

        if (db.loginUser(username,password))
            return "Login success";
        else
            return "Login fail";
    };
}


//  解析http请求
pair<string,string> parsehttpRequest(const string request){
    //  找到第一个空格确定请求方法
    size_t method_end = request.find(" ");
    //  提取方法
    string method = request.substr(0,method_end);
    //  找到第二个空格确定uri
    size_t url_end = request.find(" ",method_end+1);
    //  提取uri
    string url = request.substr(method_end+1,url_end - method_end - 1);
    //  提取请求体，对于POST来说
    
    //  返回提取出的方法和uri
    return {method,url};
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
    struct sockaddr_in listen_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;


    //  2.绑定变量
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    LOG_INFO("socket created");

    if(listen_fd == -1)
        error_handling("socket()");
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(argv[1]));
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    


    //  3.bind
    if(bind(listen_fd,(struct sockaddr*)&listen_addr,sizeof(listen_addr)) == -1){
        error_handling("bind");
        close(listen_fd);
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
    //  5.accept
    clnt_addr_size = sizeof(clnt_addr);

    //  处理请求的循环体
    while(1){

        clnt_fd = accept(listen_fd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        if(clnt_fd == -1){
            error_handling("accept");
            close(listen_fd);
        }
    
        //  读取请求
        char buffer[1024] = {0};
        if(read(clnt_fd,buffer,1024) == -1){
            error_handling("read");
            close(listen_fd);
            close(clnt_fd);
        }
        string Request(buffer);

        //  解析请求
        auto [method,uri] = parsehttpRequest(Request);

        //  处理请求
        string response_body = handlehttpRequest(method,uri,Request);

        //  发送响应
        string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
        send(clnt_fd,response.c_str(),response.size(),0);
        close(clnt_fd);
    }
    //  7.close socket
    close(listen_fd);

}

void error_handling(string err){
    perror(err.c_str());
}
