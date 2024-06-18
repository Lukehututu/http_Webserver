# http_Webserver

## V1

### 流程分析

要实现一个httpServer,首先得理解服务器和浏览器以及用户操作间的交互关系.

1. 用户通过浏览器输入一串网址,发送请求给Web服务器.
2. 服务器解析这些请求,然后根据请求的不同分别进行对应内容的响应
3. 请求的具体内容

- **请求行**

  描述HTTP方法(如GET/POST等)，URI(请求资源的路径)和版本(HTTP协议版本)的重要部分

  eg:

  ```bash
  GET /index.html HTTP/1.1
  ```

  代表用GET方法请求/index.html资源,遵循HTTP/1.1版本

- **请求头**

  提供请求的附加信息,影响处理的方法,其中还包括了`Content-Type`和`User-Agent`,其中前者指定了发送数据的类型,后者描述请求发起者的环境,如浏览器类型.

  eg

  ```bash
  Content-Type: application/json
  User-Agent:Mihoyo/5.0
  ```

- **消息体**

  可选部分,常用于POST请求中传输数据

**常见的请求类型:**

GET和POST请求

- **GET请求**

  请求获取服务器资源,参数出现在URL中

  ![image-20240618220203236](F:\project\http_WebServer\image\image-20240618220203236.png)

- **POST请求**

  提交数据给服务器,数据包含在请求体内

  ![image-20240618220350680](F:\project\http_WebServer\image\image-20240618220350680.png)

**请求的解析**

请求解析涉及提取方法,URI以及处理请求体中的数据,对于POST请求特别重要

- 解析方法

  提取请求中的HTTP方法和资源路径

- 处理数据

  对POST请求体内容进行提取和使用

eg:

```
GET /index.html HTTP/1.1
Host: www.example.comUser-Agent: Mihoyo/5.0
```

请求行为`GET /index.html HTTP/1.1`,表示这是一个GET请求,请求的资源为`/index.html`.请求头包含`Host: www.example.comUser-Agent: Mihoyo/5.0`,分别表示请求的服务器地址和客户端信息.

由于是GET请求,因此没有请求体

### 代码

```cpp
#include <iostream>
#include <string>           //  引入字符串处理函数
#include <functional>       //  引入包装器
#include <map>              //  引入标准映射容器库
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
using namespace std;

#define PORT 8080           //定义端口号
void error_handling(string err);

using RequestHandler = function<string(const string&)>; //  定义请求处理函数类型

map<string,RequestHandler> route_tables;                //定义路由表，映射路径到对应的处理函数

//  分别为GET和POST请求设置路由表
map<string,RequestHandler> get_routes;
map<string,RequestHandler> post_routes;

//  初始化路由表    --  为不同的路径设置不同的处理函数  --主要作用是填充，因此返回值是void
void setupRoutes(){
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
        //  实现用户注册逻辑
        return "Register successful";
    };
    post_routes["/login"] = [](const string request){
        //  实现用户登录逻辑
        return "Login successful";
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
    
    //  返回提取出的方法和uri
    return {method,url};
}

//  处理http请求    --  需要请求方法（method）、请求路径（uri）、请求体（body）
string handlehttpRequest(const string method,const string url,const string body){
    //  检查是否是GET请求
    if(method == "GET" && get_routes.count(url) > 0) {
        return get_routes[url](body);
    }else if(method == "POST" && get_routes.count(url) > 0) {
        return get_routes[url](body);
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
    //  设置路由
    setupRoutes();

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

```

