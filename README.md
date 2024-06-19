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

//  分别为GET和POST请求设置路由表		-- 每一个url(key)对应一个RequestHandler(value)
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
        return ;
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
        return ;
    }


    //  4.listen
    if(listen(listen_fd,5) == -1){
        error_handling("listen");
        close(listen_fd);
        return ;
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
            return ;
        }
    
    //  读取请求
        char buffer[1024] = {0};
        if(read(clnt_fd,buffer,1024) == -1){
            error_handling("read");
            close(listen_fd);
            close(clnt_fd);
            return ;
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

![v1流程](F:\project\http_WebServer\image\v1流程.png)

### 测试

在编译并运行`Server`成功后,可以通过`curl`命令进行测试,或者直接在浏览器上测试

![image-20240619125458159](C:\Users\Luk1\AppData\Roaming\Typora\typora-user-images\image-20240619125458159.png)

![image-20240619125812093](C:\Users\Luk1\AppData\Roaming\Typora\typora-user-images\image-20240619125812093.png)

## V2

在本版本中将加入日志系统.

**预期效果:**

1. **初始化日志**

   在服务端应用启动时,应该开始记录日志,录入使用`LOG_INFO`宏记录服务器的启动状态

2. **记录连接**

   当新的客户端连接时,在日志中记录,帮助追踪或与的连接和潜在的问题

3. **结束日志**

   再连接关闭时记录日志,这是确保每次连接都有完整日志记录的重要步骤

### 创建Logger

1. **定义日志级别**

   在Logger.h文件中定义日志级别,以确定消息记录的重要性和分类

   - 可能有`WARN`,`INFO`,`ERROR`等级别

2. **日志输出函数**

   根据不同的日志级别,日志内容可以被输出到不同的媒介,如控制台或文件

3. **时间戳与上下文**

   每条日志都应该包含时间戳,文件名和行号,已提供足够的信息以便回溯问题

#### 日志中宏定义的使用

1. **简化操作**

   宏定义在日志系统中用于简化记录操作

2. **捕获上下文**

   示例宏中捕获信息的文件和行号,增加了日志的详细度和追踪方便性

3. **处理可变参数**

   可变参数宏允许编写更复杂的日志信息,通过..和`va_list`处理不同情况下的日志记录

#### 该头文件的作用

定义一个函数,能够将格式化日志输出到日志文件中

再定义几个宏来简化调用操作

#### Logger.h

**先确定日志级别,先枚举出来方便后面调用**

```cpp
//  定义日志级别
enum LogLevel{
    INFO,
    WARNING,
    ERROR
};
```

**定义日志类**

声明函数

```cpp
//	因为我们的目的时封装和快捷调用,因此设计成静态方法更合适 
static void logMessage(LogLevel level,const char* format,...)
```

包含三个参数:

1. `LogLevel level` -- 用于指定日志级别
2. `const char* format` -- 用于指定和格式化输出的**字符串的模板**
3. `...` -- 可变参数列表

**确定要输出的日志中应当包含的信息**

1. 当前的时间戳
2. 日志级别
3. 日志具体内容

> 时间戳

```cpp
#include <chrono>
//  获取当前时间    
auto now = chrono::system_clock::now();                 //  返回当前时间点
auto now_c = chrono::system_clock::to_time_t(now);      //  将该时间转化为time_t类型
//	后面再通过ctime(&now_c)来转换成字符串输出
```

> 获取日志级别

```cpp
string levelStr;			//将其存储到levelStr中
switch(level){
case INFO:
	levelStr = "INFO"
    break;
case WARNING:
	levelStr = "WARNING"
    break;
case ERROR:
	levelStr = "ERROR"
	break;
}
```

> 获取格式化的具体内容

```cpp
#include <sctdarg>

//	实例化一个可变参数列表
va_list args;
//	初始化该对象
va_start(args,format);
//	要用char类型的缓存区因为vsnprintf的第一个参数接受的类型是char*,不能用string 因为.c_str()是const char*类型不符合
char buffer[2048];
//	使用格式化输出字符串将args中的内容一个一个匹配到format中然后写入buffer中
vsnprintf(buffer,sizeof(buffer),format,args);
//	释放该对象的资源
va_end(args);	
```

> 将内容拼接写入日志中

```cpp
//  将时间戳，日志级别，以及格式化后的日志信息写入日志文件
logFile << ctime(&now_c) << " [" << levelStr << "] " << buffer <<endl;		//ctime()得到的字符串自带换行符
```

最后提供几个宏方便调用

```cpp
//  定义宏以简化日志记录操作，提供三种级别的宏
#define LOG_INFO(...) Logger::logMessage(INFO,__VA_ARGS__)			
#define LOG_WARNING(...) Logger::logMessage(WARNING,__VA_ARGS__)
#define LOG_ERROR(...) Logger::logMessage(ERROR,__VA_ARGS__)
```

这个宏是==__VA_ARGS__==,前后有两个`_ _`

举例:

```cpp
#include <iostream>
#include "Logger.h"

int main(){
    LOG_INFO("hello %s %s","this","luke");
}
//	在与编译后会展开成
//	logMessage(INFO,"hello %s %s","this","luke");
//	会自动填充INFO后面的参数列表
```

![image-20240619154409829](C:\Users\Luk1\AppData\Roaming\Typora\typora-user-images\image-20240619154409829.png)

#### 完整的头文件

> Logger.h

```cpp
#include <chrono>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdarg>  //  引入处理可变参数的头文件

using namespace std;

//  定义日志级别
enum LogLevel{
    INFO,
    WARNING,
    ERROR
};


//  定义日志类
class Logger{
public:
    //  该静态成员函数用于记录日志信息
    //  参数包括日志级别，格式化字符串以及可变参数列表
    static void logMessage(LogLevel level,const char* format,...){
        //  打开日志文件，以追加的方式写入
        ofstream logFile("server.log",ios::app);

        //  获取当前时间    
        auto now = chrono::system_clock::now();                 //  返回当前时间点
        auto now_c = chrono::system_clock::to_time_t(now);      //  将该时间转化为time_t类型

        //  根据日志级别确定日志级别字符串
        string levelStr;
        switch (level)
        {
        case INFO:
            levelStr = "INFO";
            break;
        case WARNING:
            levelStr = "WARNING";
            break;
        case ERROR:
            levelStr = "ERROR";
        }

        //  使用可变参数处理日志信息的格式化
        va_list args;
        va_start(args,format);
        char buffer[2048];
        vsnprintf(buffer,sizeof(buffer),format,args);
        va_end(args);    

        //  将时间戳，日志级别，以及格式化后的日志信息写入日志文件
        logFile << ctime(&now_c) << " [" << levelStr << "] " << buffer <<endl;

        //  关闭日志文件
        logFile.close();    
    }

};

//  定义宏以简化日志记录操作，提供三种级别的宏
#define LOG_INFO(...) Logger::logMessage(INFO,__VA_ARGS__)
#define LOG_WARNING(...) Logger::logMessage(WARNING,__VA_ARGS__)
#define LOG_ERROR(...) Logger::logMessage(ERROR,__VA_ARGS__)

//  当你在代码中调用LOG_INFO("Hello,%s",name)时
//  宏会在编译阶段替换为Logger::logMessage(INFO,"Hello,%s",name)
```

## V3

### 加入登录功能以及数据库设计

