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

  ![image-20240618220203236](F:\project\http_WebServer\image\image-20240618220203236-1720856780514-1.png)

- **POST请求**

  提交数据给服务器,数据包含在请求体内

  ![image-20240618220350680](F:\project\http_WebServer\image\image-20240618220350680-1720856780523-2.png)

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

![v1流程](F:\project\http_WebServer\image\v1流程-1720856780523-3.png)

### 测试

在编译并运行`Server`成功后,可以通过`curl`命令进行测试,或者直接在浏览器上测试

![image-20240619125458159](F:\project\http_WebServer\image\image-20240619125458159.png)

![image-20240619125812093](F:\project\http_WebServer\image\image-20240619125812093.png)

## V2-logger

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

![image-20240619154409829](F:\project\http_WebServer\image\image-20240619154409829.png)

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

## V3-sqlite

### 加入登录功能以及数据库设计

 在本次项目中选用了SQLite作为数据库

**优点:**

SQLite作为轻量级的数据库,不需要单独的服务器,数据库以文件形式存储并且能直接集成到程序中

#### 常用命令

> 系统命令

```sqlite
.schema  查看表的结构
.quit    退出数据库
.exit    退出数据库
.help    查看帮助信息
.databases 查看数据库
.tables  显示数据库中所有的表的表名
```

#### 创建或打开一个数据库

```bash
sqlite3 db_name.db(或直接db_name)
//	创建一个名为db_name的数据库,如果已经存在,则是打开这个名为db_name的数据库
```

==其他语句就是普通sql,跟mysql的区别不大==

#### Database.h

##### 整个class Database的架构

- **private**
  - `sqlite3* db` 这是一个指向数据库连接对象的指针,用于保证该类在完整的生命周期内能够正常连接
- **public**
  - `Database()`一个构造函数,用于打开数据库以及建表
  - `~Database()`析构函数,用于在程序停止时关闭连接释放资源
  - `bool registerUser()`用于用户注册
  - `bool loginUser()`用于用户登录

##### 为什么用户注册和登陆的逻辑要放在该类里面?

==因为这两个逻辑要直接与数据库进行读写交互==

##### `Database()`

```cpp
//  构造函数，用于打开数据库并创建用户表
Datebase(const string& db_path) {
    //  使用sqlite3_open打开数据库
    //  &db是指向数据库连接对象的指针
    if(sqlite3_open(db_path.c_str(),&db) != SQLITE_OK) {
        //  如果数据库打开失败，就抛出数据库运行时的错误
        throw runtime_error("Failed to open database");
    }

    //  定义创建用户表的SQL语句 -- 用const char* 类型的对象来存储
    //  两个字段，用户名和密码，都是TEXT类型
    const char* sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
    char* errmsg;

    //  执行sql语句创建表
    //  sqlite3_exec用于执行SQL语句
    //  db是数据库连接对象
    //  后面的参数是回调函数和它的参数，这里不需要回调所以传0
    //  errmsg用于传递错误信息
    if (sqlite3_exec(db,sql,0,0,&errmsg) != SQLITE_OK) {
        //  如果创建表失败，抛出运行错误并附带错误信息
        string error_msg = errmsg;	//	获取错误信息
        sqlite3_free(errmsg);	//	释放错误信息内存
        throw runtime_error("Failed to create table: " + error_msg);
    }
}
```

首先通过`sqlite3_open(path,&db)`来打开一个SQLite数据库,如果数据库文件不存在,则创建一个新的数据库文件

↓

然后开始创建用户表(user),添加上了`IF NOT EXISTS`字段,这样下次再打开服务器就不会重复创建

其中只有两个字段--`username`和`password`

==细节: 为什么此处sql使用的是const char* 而非string==

因为SQLite使用C写的,很多接口都不支持string,虽然string.c_str()也行但直接用const char*更直观

但如果当前SQL语句是动态的或拼接的,那使用string会更方便

↓

然后开始执行命令通过`sqlite3_exec(db,sql,nullptr,nullptr,&errmsg)`,中间一个是回调函数,一个是函数的第一个参数.

顺带进行错误检测

因为在要抛出错误因此先将错误内存给释放,又因为要释放所以先用`error_msg`来保存错误信息.然后`sqlite3_free(errmsg`来释放用于存放错误信息的内存然后抛出异常

##### `~Database()`

```cpp
//  析构函数，用于关闭数据库连接
~Datebase() {
    //  关闭数据库连接
    //  sqlite3_close用于关闭和释放数据库连接资源
    //  db是数据库连接对象
    sqlite3_close(db);
}
```

通过`sqlite3_close()`来关闭连接同时释放资源

##### `bool registerUser()`

```cpp
//  用户注册函数    -- 需要接收前端传入的username 和 password
bool registerUser(const string& username, const string& password) {
    //  构建sql语句用于插入新用户
    string sql = "INSERT INTO users (username, password) VALUES(?, ?);";
    sqlite3_stmt* stmt;

    //  准备sql语句，预编译以防止sql注入攻击
    if (sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr) != SQLITE_OK) {
        //  如果准备语句失败，记录日志并返回false
        LOG_INFO("Failed to prepare registration SQL for users: %s", username.c_str());
        return false;
    }

    //  绑定SQL语句中的参数，防止SQL注入
    sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,password.c_str(),-1,SQLITE_STATIC);

    //  执行SQL语句，进行用户注册
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        //  如果执行失败，记录日志，清理资源并返回false
        LOG_INFO("Register failed for user: %s",username.c_str());
        return false;
    }

    //  清理资源，关闭SQL语句
    sqlite3_finalize(stmt);
    //  记录用户注册成功的日志，实际上不建议这么写日志，但这里是为了看效果
    LOG_INFO("User registered : %s with password : %s",username.c_str(),password.c_str());
    return true;
}
```

- **构建SQL语句**

​		先定义SQL插入语句,此处用string因为要修改,一般不用修改的就用const char*,在语句中用`?`作为参数占位符

​		再生成一个`sqlite3_stmt`对象,这是一个指向预编译SQL语句的指针

**↓**

- **预编译语句,然后绑定参数**

  通过`sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr)`进行预编译

  其中-1该位置的参数代表SQL语句的长度,-1代表自动计算

  而`&stmt`用来接收预编译后的SQL语句

  `nullptr`保留参数,不使用

​		再通过

	sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
	sqlite3_bind_text(stmt,2,password.c_str(),-1,SQLITE_STATIC);

​		这两个函数来绑定文本参数

​		第二个参数是参数索引,表示索引从1开始..

​		第四个参数表示字符串长度,-1表示自动计算

​		第五个参数是一个析构函数指针,这里使用`SQLITE_STATIC`表示不需要特殊处理

**↓**

- **执行SQL语句**

  此处使用`sqlite3_step()`,因为这个函数通常用来逐步执行预编译的SQL语句,适用于需要进行参数绑定查询结果处理的复杂操作

  而`sqlite3_exec()`通常是执行不需要绑定参数的简单操作

  一般进行函数操作时,正常执行的返回码一般是`SQLITE_OK`,而如果执行的是DML的SQL则一般返回`SQLITE_DONE`或者`SQLITE_ROW`等

**↓**

- **清理资源**

​		用`sqlite3_finalize(stmt)`去手动释放由`sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr)`分配内存的stmt对象,这		两个函数常常是成对出现的

##### `bool loginUser()`

```cpp
//  用户登录函数
bool loginUser(const string username,const string password) {
    //  构建查询用户密码的sql语句
    string sql = "SELECT password FROM users WHERE username = ?;";
    sqlite3_stmt* stmt;

    //  准备sql语句
    if (sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr) != SQLITE_OK) {
        //  如果准备SQL失败，打印日志并返回false
        LOG_INFO("Failed to prepare login SQL for user: %s",username.c_str());
        return false;
    }

    //  绑定用户名参数
    sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);

    //  执行SQL语句		此处一定是不等于SQLITE_ROW才有错误，因为SQLITE_ROW这个宏表示查询并用结果这才是正确的
    //					SQLITE_DONE是查询但没结果，跟SQLITE_ERROR一样都不正确
    if(sqlite3_step(stmt) != SQLITE_ROW) {
        //  如果用户名不存在，记录日志并返回false
        LOG_INFO("User not found: %s",username.c_str());
        sqlite3_finalize(stmt);
        return false;
    }

    //  获取查询后得到的密码
    const char* stored_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
    
    //  检查密码是否匹配
    //	细节 先进行判空再进行比较判断，防止后面通过该指针生成password_str出错
    if(stored_password == nullptr) {
        LOG_INFO("Stored password is null for user %s",username.c_str());
        return false;
    }
    string password_str(stored_password,sqlite3_column_bytes(stmt,0));
    sqlite3_finalize(stmt);		//	在最后一次使用完stmt后立刻释放资源
    if(password_str.compare(password) != 0) {
        //  如果密码不匹配，记录日志并返回false
        LOG_INFO("Login failed for user: %s",username.c_str());
        return false;
    }

    //  登录成功，记录日志并返回true
    LOG_INFO("User logged in: %s",username.c_str());
    return true;
}
```

- **构建SQL语句**

- **预编译语句，并绑定参数**

- **执行语句**

  这里注意返回结果是`SQLITE_ROW`

- **查询密码**

  这里要注意先判空再构造str,再进行判断

- **进行逻辑判断并返回结果**

##### `Database.h`

> Database.h

```cpp
#pragma once
#include <sqlite3.h>        //  数据库的头文件
#include <string>
#include <stdexcept>        //  错误管理的头文件

#include "Logger.h"
using namespace std;


class Database {
private:
    sqlite3* db;

public:
    //  构造函数，用于打开数据库并创建用户表
    Database(const string& db_path) {
        //  使用sqlite3_open打开数据库
        //  &db是指向数据库连接对象的指针
        if(sqlite3_open(db_path.c_str(),&db) != SQLITE_OK) {
            //  如果数据库打开失败，就抛出数据库运行时的错误
            throw runtime_error("Failed to open database");
        }

        //  定义创建用户表的SQL语句
        //  两个字段，用户名和密码，都是TEXT类型
        const char* sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
        char* errmsg =nullptr;

        //  执行sql语句创建表
        //  sqlite3_exec用于执行SQL语句
        //  db是数据库连接对象
        //  后面的参数是回调函数和它的参数，这里不需要回调所以传0
        //  errmsg用于传递错误信息
        if (sqlite3_exec(db,sql,0,0,&errmsg) != SQLITE_OK) {
            //  如果创建表失败，抛出运行错误并附带错误信息
            string error_msg = errmsg;	//	获取错误信息
            sqlite3_free(errmsg);	//	释放错误信息内存
            throw runtime_error("Failed to create table: " + error_msg);
        }
    }

    //  析构函数，用于关闭数据库连接
    ~Database() {
        //  关闭数据库连接
        //  sqlite3_close用于关闭和释放数据库连接资源
        //  db是数据库连接对象
        sqlite3_close(db);
    }

    //  用户注册函数    -- 需要接收前端传入的username 和 password
    bool registerUser(const string& username, const string& password) {
        //  构建sql语句用于插入新用户
        string sql = "INSERT INTO users (username, password) VALUES(?, ?);";
        sqlite3_stmt* stmt;

        //  准备sql语句，预编译以防止sql注入攻击
        if (sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr) != SQLITE_OK) {
            //  如果准备语句失败，记录日志并返回false
            LOG_INFO("Failed to prepare registration SQL for users: %s", username.c_str());
            return false;
        }

        //  绑定SQL语句中的参数，防止SQL注入
        sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
        sqlite3_bind_text(stmt,2,password.c_str(),-1,SQLITE_STATIC);

        //  执行SQL语句，进行用户注册
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            //  如果执行失败，记录日志，清理资源并返回false
            LOG_INFO("Register failed for user: %s",username.c_str());
            return false;
        }

        //  清理资源，关闭SQL语句
        sqlite3_finalize(stmt);
        //  记录用户注册成功的日志，实际上不建议这么写日志，但这里是为了看效果
        LOG_INFO("User registered : %s with password : %s",username.c_str(),password.c_str());
        return true;
    }

    //  用户登录函数
    bool loginUser(const string username,const string password) {
        //  构建查询用户密码的sql语句
        string sql = "SELECT password FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;

        //  准备sql语句
        if (sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr) != SQLITE_OK) {
            //  如果准备SQL失败，打印日志并返回false
            LOG_INFO("Failed to prepare login SQL for user: %s",username.c_str());
            return false;
        }

        //  绑定用户名参数
        sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);
        
        //  执行SQL语句
        if(sqlite3_step(stmt) != SQLITE_ROW) {
            //  如果用户名不存在，记录日志并返回false
            LOG_INFO("User not found: %s",username.c_str());
            sqlite3_finalize(stmt);
            return false;
        }

        //  获取查询后得到的密码
        const char* stored_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
        
        //  检查密码是否匹配
        //	细节 先进行判空再进行比较判断，防止后面通过该指针生成password_str出错
        if(stored_password == nullptr) {
            LOG_INFO("Stored password is null for user %s",username.c_str());
         return false;
        }
        
        string password_str(stored_password,sqlite3_column_bytes(stmt,0));
        sqlite3_finalize(stmt);		//	在最后一次使用完stmt后立刻释放资源
        if(password_str.compare(password) != 0) {
            //  如果密码不匹配，记录日志并返回false
            LOG_INFO("Login failed for user: %s",username.c_str());
            return false;
        }
		 //  登录成功，记录日志并返回true
        LOG_INFO("User logged in: %s",username.c_str());
        return true;
};
```

#### 将Database集成到server.cpp中

主要多了个解析请求体的函数,同时路由函数也做了一定的修改

##### `parseFromBody()`

```cpp
#include <sstream>          //  istringstream
//  从请求体中解析用户名和密码
map<string,string> parseFromBody(const string& request) {
    //  从请求中解析出请求体
    string tmp(request);
    size_t body_start = tmp.find("\r\n\r\n");		
    //	在HTTP请求中，头部字段和请求体之间的间隔符通常是"\r\n\r\n"。这是因为HTTP协议规定了使用CRLF（Carriage Return Line Feed，回车换行）作为行的结束符，而不是单独的换行符。
    string body = tmp.substr(body_start + 4);
	
    //	存储键值对
    map<string,string> params;
    //	创建一个字符串流对象`stream`,用来从字符串body中读取数据,body是请求体的具体内容
    istringstream stream(body);
    string pair;

    LOG_INFO("Parsing body: %s",body.c_str());

    //	每次读取一段字符串(截止到&,但不包括)放在pair中
    //	比如原本是username=yuanshen&password=123456
    //	第一次读取username=yuanshen放在pair中
    //	第二次没有&因此就全部读取到了pair中也就是password=123456
    while(getline(stream,pair,'&')) {
        string::size_type pos = pair.find('=');
        //	其中npos表示一个无效或不存在的位置或索引,此处用来表示没找到
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
```

==此处将body转换为字符流stream的形式来进行操作==,是因为使用getline()(该函数的第一个参数是流对象包括文件流标准输入流和字符流)很容易就能按照指定分隔符分隔字符串

##### `setupRoutes()`

```cpp
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
```

主要修改在于POST请求的处理中多了注册和登录逻辑

##### `server.cpp`

> server.cpp

```cpp
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

```

## V4-epoll

将原先的简单的server.cpp改造成epoll模型

### `epoll_server`

```cpp
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
    delete[] events;	//	释放events数组
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
```

### 重点:

最需要注意的是当要接收客户端请求的时候read返回值的处理

```cpp
else {
            //  边缘触发就靠重复读取该事件的缓冲区来获得所有数据
            string Request;
            while(1) {
                strlen = read(events[i].data.fd,buffer,BUF_SZ-1);
                if( strlen == -1) {
                    //	当全局变量errno是这两个宏时说明还有数据读,可以待会儿再读
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
                    //	一般是对方关闭了连接
                    break;
                } else {
                    buffer[strlen] = '\0';
                    Request += buffer;
                }
            }
	...
}
```

### 错误总结

1. **对于read的返回值的理解不到位,简单的说就是在ET(边缘触发)条件下,read的执行模式和errno的错误控制不熟练**

2. **打印日志或者说错误调试的水平不到位**

3. **==最简单的loginUser函数没有返回值看了半天==**

4. **回顾read的缓冲区为什么要手动设置`\0`的问题**

   1. **字符数组的使用**：
      - `buffer` 是一个字符数组，用来存储从 socket 中读取的数据。
      - C/C++ 的字符串（以 `\0` 结尾的字符序列）必须以空字符（`\0`）作为结尾来表示字符串的结束。
   2. **read 函数的返回值**：
      - `read` 函数返回读取的字节数。当我们将 `read` 的返回值 `readlen` 作为字符串结束位置时，我们需要在 `buffer` 中手动添加 `\0`，以标记字符串的结束。
      - 如果不手动设置 `\0`，那么 `Request += buffer;` 将会将 `buffer` 中所有的数据（包括未定义的部分）作为字符串添加到 `Request` 中，这可能导致 `Request` 中包含不必要的数据。
   3. **字符串操作**：
      - 在 C/C++ 中，字符串通常被处理为以 `\0` 结尾的字符数组。这种约定使得字符串操作更加方便和安全，可以使用标准库中的字符串处理函数（如 `std::string` 的成员函数）来操作字符串，而无需担心字符串的终止问题。

   因此，将 `buffer` 中的数据添加到 `Request` 字符串之前，需要确保 `buffer` 中的数据以 `\0` 结尾，这样 `Request += buffer;` 操作才能正确地将 `buffer` 中的数据作为字符串添加到 `Request` 中。

5. **对于Linux下端口占用情况的查询和如何释放端口的命令不熟练**

6. **搞清楚LT和ET要在什么情况下使用**

   1. **客户端**：
      - 通常情况下，对于客户端来说，设置边缘触发是比较常见的做法，特别是在高性能的网络编程中。这是因为边缘触发模式可以帮助客户端更高效地处理事件，减少不必要的事件通知。
   2. **服务端**：
      - 对于服务端来说，设置触发模式则需要视具体的应用场景和需求而定：
        - 如果服务端需要确保在一次事件通知中尽可能处理完所有相关数据，那么可以选择边缘触发模式。这样可以避免因为未处理完数据而频繁触发事件通知。
        - 如果服务端处理数据的逻辑可以保证每次事件通知都能完整处理，也可以选择边缘触发模式。
        - 如果服务端的处理逻辑可以分批次处理数据，并且希望通过事件通知来驱动数据处理流程，那么可以选择水平触发模式。

   ### 需要注意的点：

   - **处理逻辑和性能影响**：选择事件触发模式时，需要考虑应用程序的处理逻辑和性能特征。边缘触发模式可以减少事件通知的次数，但要求应用程序具备更为高效的数据处理能力。
   - **错误处理**：无论选择哪种模式，都需要正确处理错误和异常情况，确保应用程序能够稳定可靠地运行。

   综上所述，服务端在设置 socket 的事件触发模式时，应根据具体需求来选择合适的模式，通常情况下边缘触发是一个不错的选择，但要确保应用程序能够正确处理边缘触发带来的一次性事件通知。

## V5_threadpool

添加线程池

> thread.hpp

```cpp
#pragma once
#include<vector>                        //存储工作线程
#include<queue>                         //存储待执行的任务
#include<thread>                        //引入线程库，用于创建和管理线程
#include<mutex>                         //引入互斥锁，用于确保线程安全
#include<condition_variable>            //引入条件变量，用于线程等待和通知
#include<functional>                    //引入函数对象包装库，用于可调用对象包装器
#include<future>                        //用于管理异步任务的结果
using namespace std;



class ThreadPool {
public:
    //构造函数，初始化线程池
    ThreadPool(size_t threads): stop(false) {
        //创建指定数量的工作线程
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this]  {
                while(true) {
                    function<void()> task;
                    //  此处并不是单纯指一个代码块，更是指一个作用域，具体作用是让lock在该代码块后就自动销毁，
                    //  让锁的时间尽可能减少
                    {
                        LOG_INFO("Thread%d waiting...",this_thread::get_id());
                        //  创建互斥锁以保护任务队列
                        unique_lock<mutex> lock(this->queue_mutex);
                        LOG_INFO("Thread%d acquired lock",this_thread::get_id());
                        //  使用条件变量等待任务或停止信号 -- 等待任务中condition.notify_one();去唤醒
                        /*  此时线程都先释放锁（因此锁在没任务的时候是没人拥有的），然后检验predicate，一开始都是false，因此先阻塞，直到
                            接收到notify_one 然后重新获得锁，然后检查，此时任务队列不为空因此跳出*/
                        this->condition.wait(lock,[this]{ return this->stop || !this->tasks.empty(); });
                        LOG_INFO("thread wake up...");
                        //  如果线程池停止且任务队列为空，则线程退出
                        if(this->stop && this->tasks.empty()) return ;
                        LOG_INFO("Thread%d release exit...",this_thread::get_id());
                        //  否走就正常获取下一个要执行的任务
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }
                    //  执行任务
                    task();
                }
            });
        }
    }

    //提交任务到线程池的模板方法

    //将函数调用包装为任务并添加到任务队列，返回与任务关联的 future
    /*
    这段代码是C++中用于创建一个异步任务处理函数模板的部分，该函数模板接收一个可调用对象f和一组参数args...
    并返回一个与异步任务结果关联的future对象，以下是详细解释：

    future<typename result_of<F(Args...)>::type>

    future: C++标准库中的组件，用于表示异步计算的结果
    当你启动一个异步计算时，可以获取一个future对象，通过它可以在未来某个时间点查询异步操作是否完成，并获取其返回值

    typename result_of<F(Args...)>::type;
    这部分是类型推导表达式，使用了C11引入的result_of模板
    result_of可以用来确定可调用对象F在传入参数列表Args...时的调用结果类型
    在这里，它的作用是推断出函数f(args...)调用后的返回类型

    整体来看，这个返回类型的声明意味着enqueue函数的返回一个future对象
    而这个future所指向的结果数据类型正是由f(args...)所得到的类型

    using return_type = typename result_of<F(Args...)>::type;

    using关键字在这里定义了return_type是后面的别名(后面的太长了)

    在后续的代码中，return_type将被用来表示异步任务的返回类型，简化代码并提高可读性
    */
   template<class F,class... Args>                                          //  此时是任务队列无限大因此就没有用Maxqueue,也可以在其中添加判断的逻辑和调度的逻辑                                            
   auto enqueue(F&& f,Args&& ... args) -> future<typename result_of<F(Args...)>::type> {
        using return_type = typename result_of<F(Args...)>::type;
        //  创建共享任务包装器
        /*
        1.packaged_task<return_type()>
        packaged_task是一个c++模板库中的类模板，它能够封装一个可调用对象（如函数、lambda表达式或函数对象）
        并将该对象的返回值存储在一个可共享的状态中，这里的return_type()表示任务的返回值是无参数的
        并且有一个return_type的返回类型

        2.make_shared<packaged_task<return_type()>>      
        make_share是一个工厂函数，用于创建一个share_ptr实例，指向动态分配的对象  
        在这里，它被用来创建一个指向packaged_task<return_type()>类型的实力的智能指针
        而make_shared的好处在于它可以一次性分配所有需要的内存（包括管理块和对象本身），从而减少内存分配次数并提高效率，同时确保资源正确释放

        3.bind(forward<F>(f),forward<Args(args)...)
        创建一个新的可调用对象，新的可调用对象可以在之后的任意时刻以适当的方式调用原始可调用对象
            - F代表传入的可调用对象类型，Args代表有可能的参数列表
            - forward<F>(f),forward<Args(args)...是对完美转发的支持
            使得f和args能以适当的左值引用或右值引用的形式传递给bind
            这样可以保持原表达式的值类别信息，特别是当传递的是右值时，可以避免拷贝开销
        */
        auto task = make_shared<packaged_task<return_type()>>(
            bind(forward<F>(f),forward<Args>(args)...)
        );
        //  获取与任务关联的  future
        future<return_type> res = task->get_future();
        {
            //  使用互斥锁保护任务队列
            unique_lock<mutex> lock(queue_mutex);
            //  如果线程池已经停止，则抛出异常
            if(stop) throw runtime_error("enqueue on stopped ThreadPool");
            //  将任务添加到队列
            tasks.emplace([task](){(*task)(); });
        }
        //  通知一个等待的线程去执行任务
        condition.notify_one();
        LOG_INFO("notify_one");
        return res;
    }

    ~ThreadPool(){
        {
            //  使用互斥锁保护停止标志
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        //  唤醒所有等待的线程
        condition.notify_all();
        //  阻塞主线程然后等待所有工作线程退出
        for(thread& worker : workers) {
                worker.join();
        }
    }

private:
    vector<thread> workers;             //存储工作线程
    queue<function<void()>> tasks;      //存储任务队列
    mutex queue_mutex;                  //任务队列的互斥锁
    condition_variable condition;       //条件变量用于线程等待
    bool stop;                          //停止标志，用于控制线程池的生命周期
    //size_t Maxqueue;                    //最大进队数量
};
   
//  对于模板类来说，如果声明和定义在两个文件里，那main.cpp中应该两个头文件都包含
```

集成到main.cpp中

```cpp
//  返回静态资源
string getStaticRec() {
    ifstream ifs;


    ifs.open("../index.html",ios::binary | ios::in);

    if(!ifs.is_open()) {
        cerr<<"文件打开失败"<<endl;
        return nullptr;
    }

    //  得到buf
    //  获取文件长度
    //  seekg(0,ios::end) 表示将文件指针自end位置偏移0字节--也就是置于文件尾
    ifs.seekg(0,ios::end);
    //  tellg() 表示从文件指针的位置到文件头位置的字节数
    length = ifs.tellg();
    //  再将文件指针复位至文件开头以正常操作文件
    ifs.seekg(0,ios::beg);

    char buf[length];
    ifs.read(buf,length);

    return string(buf);
}


//	添加了返回静态资源的路由
void setupRoutes(){
  	//...
    get_routes["/index.html"] = [](const string body){
        //  直接返回静态资源
        return getStaticRec();
    };
    //...
}


int main(int argc,char* argv[]){

   	//	...
    
    //  创建epoll实例
    int ep_cnt = 0;
    int epfd = epoll_create(EPOLL_SZ);
    if( epfd == -1) {
        LOG_ERROR("epoll_create -1");  //  注册事件失败，记录错误日志
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
    
    ThreadPool pool(4);
    //  处理请求的循环体
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
                setNonBlocking(clnt_fd);    //  设置新连接为非阻塞模式 -- 以便后续再epoll中进行异步IO操作
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
                int fd = events[i].data.fd;
                pool.enqueue([fd] {
                    char buffer[BUF_SZ];
                    ssize_t strlen = 0;
                    //  边缘触发就靠重复读取该事件的缓冲区来获得所有数据
                    string Request;
                    while(1) {
                        strlen = read(fd,buffer,BUF_SZ - 1);
                        if( strlen == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                //  没有数据可读了
                                break;
                            } else {
                                //  此时真正的出错了
                                error_handling("read");
                                close(fd);
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
                    string response;
                    if(url != "/index.html") {
                        response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
                    } else {
                        response = "HTTP/1.1 200 OK\nContent-Type: text/html\r\nContent-Length: " + to_string(length) + "\r\n\r\n" + response_body;
                    }
                    send(fd,response.c_str(),response.size(),0);
                    //  将其移除epoll池
                    //epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
                    close(fd);
                    LOG_INFO("Request handled and response sent on socket: %d",fd);
                });   
            }
        }
    }
    //  7.close socket
    close(listen_fd);
    close(epfd);
    delete[] events;
}

```

分析:

```cpp
首先,循环threads,从而创建threads个线程.然后将每个线程推入workers数组中

workers.emplace_back(thread);
```

**为什么此处使用emplace_back而非push_back?**

在C++中，`emplace_back` 和 `push_back` 都用于向 `std::vector` 添加元素，但它们在使用方式和性能方面有一些重要区别。了解这些区别有助于理解为什么在这个场景中选择 `emplace_back` 而不是 `push_back`。

#### `push_back` 与 `emplace_back` 的区别

1. **`push_back`**：

   - `push_back` 将一个已经构造好的对象添加到 `std::vector` 的末尾。
   - 这个对象通常是通过拷贝或移动的方式添加的，这意味着可能会有额外的构造和析构开销。

   示例：

   ```cpp
   std::vector<std::thread> workers;
   std::thread t([] { /* thread function */ });
   workers.push_back(std::move(t)); // 需要移动构造
   ```

2. **`emplace_back`**：

   - `emplace_back` 直接在 `std::vector` 的末尾构造对象。
   - 这避免了额外的拷贝或移动操作，因为对象是直接在目标位置构造的。
   - `emplace_back` 接受构造对象所需的参数，并将这些参数直接传递给对象的构造函数。

   示例：

   ```cpp
   std::vector<std::thread> workers;
   workers.emplace_back([] { /* thread function */ }); // 直接构造
   ```

#### 为什么在这个场景中使用 `emplace_back`

在线程池的实现中，使用 `emplace_back` 而不是 `push_back` 有几个明显的优势：

1. **避免不必要的移动**：
   - 使用 `emplace_back` 可以直接在 `workers` 容器中构造线程对象，而不是先构造一个临时的 `std::thread` 对象然后再移动它到容器中。这减少了一次移动构造的开销。

2. **简洁性**：
   - 使用 `emplace_back` 可以直接传递构造线程所需的参数（如 lambda 表达式），代码更简洁明了。
   - 省去了显式构造临时对象的步骤，使代码更直观。

3. **性能**：
   - 直接在容器内部构造对象可以避免额外的构造和析构开销，提升性能。
   - 特别是在涉及大对象或复杂构造函数时，这种性能提升更为明显。



- **`emplace_back` 使用 lambda 表达式直接构造线程**：
  通过 `emplace_back`，我们直接在 `workers` 向量的末尾构造一个 `std::thread` 对象，并且传递了一个 lambda 表达式作为其构造函数的参数。这样做避免了临时对象的创建和移动。

- **简化了代码结构**：
  通过使用 `emplace_back`，代码更加简洁。我们不需要显式创建一个 `std::thread` 临时对象并移动它到向量中，这使代码更易读，更直观。

使用 `emplace_back` 而不是 `push_back` 的主要原因在于：

- **性能优化**：避免了不必要的对象移动或拷贝。
- **代码简洁**：直接传递构造参数，简化代码结构。

在实现线程池时，使用 `emplace_back` 可以更高效地管理线程对象，提升性能并保持代码简洁。

*****************************************

**其次,不难发现,emplace_back()的参数是一个lambda表达式,这是因为:**

`std::thread` 是一个线程类，它的构造函数可以接受一个可调用对象（callable object）作为参数。

##### `std::thread` 的构造函数

`std::thread` 的构造函数可以接受一个可调用对象，并在线程开始时执行它。例如：

```cpp
std::thread t([]{ std::cout << "Hello, world!" << std::endl; });
```

这个构造函数将会创建一个新线程，并执行传递的 lambda 表达式中的代码。

`emplace_back` 的参数是一个 lambda 表达式，它被传递给 `std::thread` 的构造函数，从而在 `workers` 向量的末尾构造一个新的 `std::thread` 对象。**这个新创建的线程将会执行 lambda 表达式中的代码。**

##### 为什么这样设计

这种设计的好处是简洁且高效。通过 lambda 表达式，可以直接在 `emplace_back` 调用中定义线程的行为，而无需单独定义一个函数或仿函数类。这种方式使代码更简洁明了，并且能够直接访问线程池对象的成员变量和成员函数。

#### 分析单个线程的执行过程

```cpp
while(true) {
    ----------------------------------------------------------
                    function<void()> task;
                    {
                        //  创建互斥锁以保护任务队列
                        unique_lock<mutex> lock(this->queue_mutex);
                        //  使用条件变量等待任务或停止信号
                        this->condition.wait(lock,[this]{return this->stop || !this->tasks.empty();});
                        //当前线程或者但任务队列为空时,线程才休眠释放锁,不占用cpu资源 
                        //  如果线程池停止或任务队列为空，则线程退出
                        if(this->stop && !this->tasks.empty()) return ;
                        //  否走就正常获取下一个要执行的任务
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }
    //因为ThreadPool的tasks是共享队列,因此是临界资源因此要加锁
	----------------------------------------------------------
                    //  执行任务
                    task();
                }
```

### 分布式服务器：异步处理与缓存机制详细教程

#### `async`

> 函数签名

````cpp
template< class Function, class... Args >
std::future< typename std::result_of<Function(Args...)>::type >
async( std::launch policy, Function&& f, Args&&... args );
//参数:
一个可选的标志，用于指定任务的启动策略（std::launch）。
一个可调用对象（如函数指针、函数对象或lambda表达式）。
可调用对象的参数。

//返回类型
返回一个future对象
可能是future<int> 可能是future<string>...
````

> 使用async启动一个异步任务

```cpp
auto future = async(launch::async,[](){
    return "Hello from async!" ; 
});

//此时我们的策略是launch::async -- 立即启动一个新线程
可调用对象是一个lambda表达式,没有参数
    
    
//策略还可以是launch::deferred:延迟执行任务,直到第一次通过future.get()或future.wait()显式请求结果
```

> 获取异步结果--future的get方法

```cpp
string result = future.get();
cout<<result<<endl; 	//输出-->"Hello from async!"
```

如果异步操作尚未完成,则调用`future.get()`会阻塞当前线程,直到读取到异步结果

#### future

- **任务状态查询**

  future提供了**对异步任务的状态查询能力**,能够有效的知晓任务是否已完成或仍在执行中

- **结果获取**

  当异步操作完成后,可以通过`get()`方法获取异步处理的结果,**若没准备好则会阻塞**

- **异常处理**

  倘若在异步处理过程中发生异常,**future会将异常捕获,并在调用`get()`时重现该异常**

#### 异步操作应用场景

- **Web服务器**

  在Web服务器中,async能处理并发的客户端请求,每个请求能在独立的线程中异步处理,提升整体吞吐量

- **数据库操作**

  数据库读写操作经常是系统中延迟的瓶颈,通过异步化可以减少等待时间,加快数据处理流程

- **长时间运算**

  对于计算密集型任务,async可以放置后台执行,同时释放前端线程与处理其他的用户交互

 

**但本项目为什么没有使用异步操作?**

因为本项目只有简单的注册登录操作,这种操作简单,如果在有大量请求访问的情况下,异步操作因为操作简单会大量的创建线程然后销毁线程,这样开销也很大,因此就不用异步操作

## V6_demo

### 模块化

这个版本主要把冗杂的main.cpp给分离成不同的功能，将功能模块化来减少后期维护和再开发的成本,同时简化main.cpp

首先搞清楚整个服务器的工程结构

> tree

```bash
.
├── build
├── CMakeLists.txt
├── Database.hpp			-- 1
├── HttpRequest.hpp			-- 2
├── HttpResponse.hpp		-- 3 
├── HttpServer.hpp			-- 4
├── Logger.hpp				-- 5
├── main.cpp				-- 6 
├── Router.hpp				-- 7
├── static_rc
│   ├── index.html
│   ├── login.html
│   └── register.html
└── ThreadPool.hpp			-- 8
```
#### **`HttpRequest`类**

此类负责解析客户端的请求,提取出方法,路径,以及表单数据等信息

```cpp
#pragma once
//  该类解析来自客户端的请求，提取出关键信息，例如  请求方法   ，路径，以及 提交的数据  等
#include <string>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include "Router.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
using namespace std;


class HttpRequest {
public:
    enum Method {
        GET, POST, HEAD, PUT, DELETE, OPTIONS, CONNECT, PATCH,UNKNOW
    };
    enum ParseState {
        REQUEST_LINE, HEADERS, BODY,FINISH
    };
    
    HttpRequest(): method(UNKNOW),state(REQUEST_LINE){}
    
    //  一个POST请求示例
    /*
    POST /login HTTP/1.1                                -- parseRequestLine(line)
    HOST: localhost:8080                                -- parseHeader(line)    
    Content-Type: application/x-www-form-urlencoded     -- parseHeader(line)
    Content-Length: 30                                  -- parseHeader(line)

    username=yuanshen&password=test1
    */
	//	除了第一行是用空格来分割,请求头后面的部分都是": "来分割的形式

    //  传入读到的缓冲区的数据--buffer，并将其解析
    bool parse(const string& request) {
        istringstream iss(request);
        string line;
        bool result = true;

        //  按行读取请求，并根据当前解析状态处理
        //  循环条件 -- 首先能有line 然后line不是换行符
        while(getline(iss,line) && line != "\r") {
            if(state == REQUEST_LINE) {
                result = parseRequestLine(line);    //  解析请求行
            } else if (state == HEADERS) {
                result = parseHeader(line); //  解析请求头
            }
            if (!result) {
                break;  //  如果解析失败则跳出循环
            }
        }

        //  如果请求方法是POST，则提取请求体
        if (method == POST) {
            body = request.substr(request.find("\r\n\r\n") + 4);
        }

        return result;  //  返回解析结果
    }

    //  从请求体中解析用户名和密码      解析用户传来的表单
    unordered_map<string,string> parseFromBody() const {
        unordered_map<string,string> params;
        if(method != POST) return params;   

        istringstream stream(body);
        string pair;

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

    //	以下的两个函数我只希望返回值不希望改变任何的对象,因此用const放末尾来修饰
    	
    //  获取Http请求方法的字符串表示
    string getMethodString() const {
        switch(method) {
        case GET:   return "GET";
        case POST:  return "POST";
        //  之后添加其他的方法
        //  ...
        default:    return "UNKNOW";
        }
    }
	/*
	在函数声明的末尾使用 const，表示该函数是一个常量成员函数（const member function）。这种函数不能修改类的任何成员变量，也不能调用非 const 成员函数。它可以提高代码的安全性，因为它保证了该函数不会改变对象的状态。
	*/
    
    //  获取请求路径的函数
    const string& getPath() const {
        return this->url;
    }

    //  其他成员函数和变量
    //  ...

private:
    //	以下两个函数是通过公共成员函数在函数内调用的,因此写在私有中
    
    bool parseRequestLine(const string line) {
        istringstream iss(line);
        string method_str;
        
        /*
        POST /login HTTP/1.1                                -- parseRequestLine(line)
        */
        //	对于流对象而言，>> 是用来提取字段的(也就是被空格包裹的字符串) 每提取一次,指针都会向后移动
        
        iss >> method_str;	//	此时就是将第一个字段POST放进了method_str中然后指针向后移动
        if(method_str == "GET") method = GET;
        else if(method_str == "POST")  method = POST;
        else method = UNKNOW;
		
        //	此时就是将/login放进了url中然后指针向后移动
        iss >> url;         //  解析请求路径
        //	此时就是将HTTP/1.1放进了version中然后指针向后移动
        iss >> version;     //  解析Http协议版本
        state = HEADERS;    //  更新状态码为解析请求头
        return true;
    }


    //  解析请求头的函数
    bool parseHeader(const string& line) {
        size_t pos = line.find(": ");
        if (pos == string::npos) {
            return false;   //  请求头格式错误
        }
        string key = line.substr(0,pos);
        string value = line.substr(pos + 1);
        headers[key] = value;   //  存储键值对到headers字典里
        return true;        
    }


    Method method;  //  请求方法
    string url;     //  请求路径
    string version; //  Http协议版本
    string body;    //  请求体
    ParseState state;      //  请求解析状态
    unordered_map<string,string> headers;   //  请求头

};
```

#### **`HttpResponse`类**

用于构建服务器的响应,包括状态码,响应头,响应体等信息

```cpp
#pragma once
//  该类构建要发回给客户端的响应，包括状态码，响应头，响应体等
#include <string>
#include <sstream>
#include <unordered_map>
using namespace std;

class HttpResponse {
public:

    HttpResponse(int code = 200): statusCode(code) {}

    //  设置状态码
    void setStatusCode(int& code) {
        this->statusCode = code;
    }

    //  设置响应体
    void setBody(const string& body) {
        this->body = body;
    }

    //  设置响应头
    void setHeader(const string& name,const string& value) {
        headers[name] = value;
    }

    //  一个响应数据的例子
    /*
        HTTP/1.1 200 OK
        Content-Type: text/html
        Content-Length: 137

        <html>
        <head>
            <title>Sample Page</title>
        </head>
        <body>
            <h1>Sample HTTP Response</h1>
            <p>This is a sample response body.</p>
        </body>
        </html>
    */


    //  将响应转换成响应信息结构的字符串，按如上的格式拼接
    string toString() const {
        ostringstream oss;
        //  添加Http头信息：版本协议 状态码 状态消息
        oss << "HTTP/1.1 " << statusCode << " " << getStatusMessage() << "\r\n";
        //  添加其他响应头
        for(const auto& header : headers) {
            oss << header.first << ": " << header.second << "\r\n";
        }

        //	手动添加实体头部
        oss << "Content-Length: " << body.size() << "\r\n";
        
        //  添加空分行间隔响应头和响应体
        oss << "\r\n" <<body;
        return oss.str();
    }
    // -------------------------------------------------------------


    //  生成错误请求
    static HttpResponse makeErrorResponse(int code,const string& error) {
        HttpResponse response(code);
        response.setBody(error);
        return response;
    }
    
    //  生成正确请求
    static HttpResponse makeOKResponse(const string sta) {

    }


private:
    
    //	简单的转换器,枚举类型转换成字符串
    //  获取状态信息(根据状态码)
    string getStatusMessage() const {
        switch(statusCode) {
        case 200: return "OK";  //  请求成功，一切正常
        case 404: return "Not Found";   //  找不到请求的资源
        case 302: return "Moved Permanently";   //资源临时重定向
        case 400: return "Bad Request"; //  客户端请求无法解析或有语法试错法
        case 204: return "Unauthorized";    //  未授权，需要有效的身份凭证
        default: return "Unknown";  //  默认是未知
        }
    }
    
    int statusCode;    //  状态码
    string body;    //  响应体
    unordered_map<string,string> headers;  //  响应头信息

};

/*	
	unordered_map：
    使用哈希表实现，没有固定的顺序，插入和查找的时间复杂度平均为 O(1)。
    对于需要快速插入和查找的场景，特别是当不需要保持顺序时，unordered_map 是一个不错的选择。
    不会按照任何特定顺序迭代元素。
    
    map：
    使用红黑树实现，元素按照键的大小顺序排列，默认升序。插入和查找的时间复杂度为 O(log n)。
    当需要按照特定顺序（例如键的字典序）访问元素时，使用 map 更合适。
    迭代元素按照键的升序顺序。
    
    在HTTP响应类中，通常头部字段的顺序对于HTTP协议来说并不是强制要求，因此如果只是存储键值对，并不需要按照特定顺序访问和迭代，使用 unordered_map 可以提供更好的插入和查找性能。这样可以在保持简单性的同时，有效地管理和处理多个头部字段。

    如果需要确保头部字段按照特定顺序输出，可以在构造 toString 方法时，手动控制输出顺序，无论是使用 unordered_map 还是 map 都可以实现。
*/
```

#### **`Router`类**

根据请求的路径和方法,决定有哪个处理函数来执行请求的具体逻辑

```cpp
//  该类主要根据请求的路径和方法，决定调用哪个处理函数，实现请求的具体逻辑
#pragma once
#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include "Database.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
using namespace std;


class Router {

public:
    
    //  定义处理函数的类型
    using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;


    //  添加路由，将    "method|url"    的组合当作routes的key
    void addRoute(string method,string url,RequestHandler func) {
        //  判断方法类型，给对应的方法加上处理函数
        routes[method + "|" + url] = func;
        //  ...more methods
    }
    

    
    //  设置数据库有关的路由
    void setupDatabaseRoutes(Database& db) {
        
        //  POST登录和注册 -- 获取表单数据
        addRoute("POST","/register" ,[&db,this](HttpRequest request) {
            HttpResponse response;

            //  类型都是html因此写在前面
            response.setHeader("Content-Type","text/html");

            //  根据注册结果返回不一样的html
            auto parm = request.parseFromBody();
            string username = parm["username"];
            string password = parm["password"];
            //  调用db的方法进行注册
            if(db.registerUser(username,password)) {
                response.setStatusCode(302);    //  302 -- 重定向
                response.setHeader("Location","/login");    //  重定向到登录界面
                return response;
            } else {
                return HttpResponse::makeErrorResponse(400,"Register Failed");
            }
        });
        
        addRoute("POST","/login" ,[&db](HttpRequest request) {
            HttpResponse response;
            response.setHeader("Content-Type","text/html");

            //  根据登录结果返回不一样的html

            //  通过request获取密码和账号的数据
            auto parm = request.parseFromBody();
            string username = parm["username"];
            string password = parm["password"];

            //  调用db的方法进行登录
            if(db.loginUser(username,password)) {
                response.setStatusCode(200);
                response.setBody("<html><body><h2>Login Successed</h2></body></html>");
                return response;
            } else {
                response.setStatusCode(401);    //  401--未授权 Unauthorized
                response.setBody("<html><body><h2>Login Failed</h2></body></html>");
                return response;
            }
        });
    }

    //  通过传进来的request来分配处理函数		相当于之前的requestHandler
    HttpResponse routeRequest(HttpRequest& request) {
        string key = request.getMethodString() + "|" + request.getPath();
        //  判断是否有相应的路由
        if(routes.count(key) > 0) {
            return routes[key](request);
        }
        
        //  如果没有找到对应的路由那就404
        return HttpResponse::makeErrorResponse(404,"Not Found");
    } 

private:
    string readfile(string file_path) {
        ifstream ifs;
        ifs.open(file_path,ios::binary | ios::in);
        if(!ifs.is_open()) {
            return nullptr;
        }

        //  得到buf
        //  获取文件长度
        //  seekg(0,ios::end) 表示将文件指针自end位置偏移0字节--也就是置于文件尾
        ifs.seekg(0,ios::end);
        //  tellg() 表示从文件指针的位置到文件头位置的字节数
        size_t length = ifs.tellg();
        //  再将文件指针复位至文件开头以正常操作文件
        ifs.seekg(0,ios::beg);

        char buf[length];
        ifs.read(buf,length);
        return string(buf); 
    }

    unordered_map<string,RequestHandler> routes;    //  存储路由映射

};
```

#### **`HttpServer`类**

```cpp
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
        ThreadPool pool(16);    //  创建线程池

        //  初始化epoll_event数组
        auto events = new epoll_event[max_events];

        //  主循环
        while(1) {
            int nfds = epoll_wait(epoll_fd,events,max_events,-1);   //  开始监听
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

     //  初始化路由
    void setupRoutes() {
        this->router.addRoute("GET","/",[this](const HttpRequest& req){ 
            //  添加根路由处理器，返回"Hello World"响应
            HttpResponse response;
            response.setBody("Hello World");
            response.setStatusCode(200);
            return response;
        });

        //  设置与数据库有关的路由
        router.setupDatabaseRoutes(this->db);
        //  其他路由在这里添加...
        
        //  GET登录和注册 -- 获取静态资源           但实际上对于get请求的处理函数不传request也没事，因为没用到
        addRoute("GET","/login",[this](HttpRequest request) {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type","text/html");
            response.setBody(readfile("../static_re/login.html")); //  读取html文件
            return response;
        });
        addRoute("GET","/register",[this](HttpRequest request) {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type","text/html");
            response.setBody(readfile("../static_re/register.html")); //  读取html文件
            return response;
        });
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
        serveraddr.sin_port = htons(PORT);
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(server_fd,(struct sockaddr*)&serveraddr,socklen) == -1) {
            LOG_INFO("bind failed");
            exit(EXIT_FAILURE);
        }

        if(listen(server_fd,5) == -1) {
            LOG_INFO("listen failed");
            exit(EXIT_FAILURE);
        }
        LOG_INFO("Server listening on port %d",PORT);   //记录服务器监听

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
            send(fd,response_str.c_str(),sizeof(response_str),0);
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
```

#### `main.cpp`

配置端口号 -> 设置数据库 -> 配置服务器对象 -> 设置路由 -> 开启服务器

```cpp
#include "HttpServer.hpp"
#include "Database.hpp"

int main(int argc,char* argv[] ) {
    int port = 8080;
    if(argc > 1) {
        port = stoi(argv[1]);
    }
    Database db("user.db");
    HttpServer server(port,10,db);
    server.start();
    return 0;
}
```

Logger\Databash\ThreadPool和之前的一样

==状态码==

**登录失败：**

- **401 Unauthorized**: 表示请求需要用户验证。通常用于未提供身份验证凭据或者凭据无效。
- **403 Forbidden**: 表示服务器已经理解请求，但是拒绝执行它。通常用于权限不足的情况。

**注册失败：**

- **400 Bad Request**: 表示客户端发送的请求有错误，通常用于请求格式错误或缺少必要参数。
- **409 Conflict**: 表示请求与服务器当前的状态冲突。通常用于注册时用户已存在的情况。

#### 流程梳理

![main](F:\project\http_WebServer\image\main-1720856780523-5.png)



![HttpServer](F:\project\http_WebServer\image\HttpServer-1720856780523-4.png)

![Router](F:\project\http_WebServer\image\Router-1720856780523-7.png)

![HttpResponse](F:\project\http_WebServer\image\HttpResponse-1720856780523-6.png)

![HttpRequest](F:\project\http_WebServer\image\HttpRequest.png)



### 前后端联动机制

![未命名绘图.drawio](F:\project\http_WebServer\image\未命名绘图.drawio-1720856780523-8.png)

## V7_Nginx

### 分布式服务器

#### Nginx

**Nginx作为反向代理的流程**

1. **客户端请求**

   请求首先到达Nginx

2. **接收请求**

   Nginx作为反向代理接收Http请求

3. **请求转发**

   Nginx根据配置转发到后端服务器

   ![Nginx](F:\project\http_WebServer\image\Nginx.png)

Nginx本身也是一个事件驱动模型

##### 负载均衡策略与Nginx实现

| 方法           | 实现                           |
| :------------- | ------------------------------ |
| 轮询           | 请求依次分配给每个服务器       |
| 加权轮询       | 依权重分配请求量               |
| 最少连接数     | 新请求到当前连接数最少的服务器 |
| 权重最少连接数 | 综合权重考虑服务器连接数       |
| 源地址哈希     | 根据请求源IP分配服务器         |

- **加权轮询**
  - 在轮询的基础上,给每个服务器设置一个权重.服务器接受的请求数量按照其权重比例分配
  - **优点:** 可以根据服务器的性能和处理能力调整权重,实现更合理的负载均衡
  - **缺点:** 权重设置需要根据服务器性能手动调配

```nginx
upstream myapp {
    server server1.example.com weight=3;
    server server2.example.com weight=2;
    server server3.example.com weight=1;
}
```

- **最少连接**
  - 新的请求会被分配给当前连接数最少的服务器
  - **优点:** 能较好地应对不同负载的情况,合理分配请求
  - **缺点:** 在高并发情况下,统计和更新连接数可能成为新的性能瓶颈

```nginx
upstream myapp {
    least_conn;
    server server1.example.com;
    server server2.example.com;
    server server3.example.com;
}
```

- **加权最少连接**
  - 在最少链接的算法基础上,考虑服务器的权重,权重越高的服务器可以承担更多的连接
  - **优点:** 相比最少链接法,能够考虑到服务器的性能
  - **缺点:** 算法相对复杂,需要维护更多的状态信息

```nginx
upstream myapp {
    least_conn;	?????????????????????????
    server server1.example.com;
    server server2.example.com;
    server server3.example.com;
}
```

- **基于源地址哈希**
  - 根据请求的源IP地址进行哈希,然后根据哈希结果分配给特定的服务器
  - **优点:** 保证来自同一源地址的请求总是被分配到同一服务器,有利于会话保持
  - **缺点:** 当服务器数量变化时,分配模式就会发生变化,可能会打乱原有的分配规则

```nginx
upstream myapp {
    ip_hash;
    server server1.example.com;
    server server2.example.com;
    server server3.example.com;
}
```

#####  Nginx缓存

​		缓存可以提高网站性能,减轻服务器负载.通过在响应请求("GET")时将数据缓存在内存中,可以减少对后端服务器的请求次数,提高用户的访问速度.可以通过在`nginx.conf`中配置`proxy_cache`指令来设置缓存的规则,包括**缓存的有效期,缓存文件路径**等.

##### 为什么本项目中只处理`GET`请求,不处理`POST`请求?

​		首先,因为`Nginx`实际上是==基于识别的请求头==来处理缓存和返回内容的,因此当传来一个`POST`请求时,例如用户注册一个账号,如果处理`POST`请求,他会先将用户数据发给服务器,服务器再发给DB,检查数据合法性然后存进DB里,若是注册成功就返回一个成功,服务器再将成功返回给`Nginx`,`Nginx`再将注册成功的消息传给用户,同时将成功这条信息缓存,当下次再有别的用户发送`POST`请求时,`Nginx`识别到是`POST`就直接返回缓存中的成功请求了,因此就出错了.

#### 配置nginx

##### `nginx.conf`

```nginx
#	1.***************************************
user root;
#	2.***************************************
events {
    worker_connections 1024; #   指定每个worker进程可以建立的最大连接数为1024
}

http {
    #   配置代理缓存路径和规则
    proxy_cache_path /var/cache/nginx levels=1:2 keys_zone=my_cache:10m max_size=10g inactive=60m use_temp_path=off;
    #   /var/cache/nginx    缓存存储的路径
    #   levels=1:2  缓存文件的目录层级
    #   keys_zone=my+cache:10m  缓存键区域名为 my_cache 大小为10MB
    #   max_size=10g    最大缓存大小为10GB
    #   inactive=60m    在60分钟内未被访问的缓存会被清除
    #   use_temp_path=off   不使用临时路径存储缓存数据

    #   定义一个名为myapp的upstream，用于负载均衡
    upstream myapp {
        #   分别是  地址    和    权重
        server localhost:8080 weight=3;     
        server localhost:8081 weight=2;
        server localhost:8082 weight=1;
    }

    #   server块定义了一个服务
    server {
        #   监听80窗口
        listen 80;  

        #   相对路径的配置
        location / {
            #   将请求转发到名为myapp的upstream
            proxy_pass http://myapp;    
            #   设置http头部，用于将客户端的相关信息转发给服务器
            proxy_set_header Host $host;    #   传递原始请求的HOST头部
            proxy_set_header X-Real-IP $remote_addr;    #   传递客户端的真实IP
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;    #   传递X-Forwarded-For头部
            proxy_set_header X-Forwarded-Proto $scheme;    #   传递原始请求使用的协议
        }

        #   /login 和 /register的   GET    请求配置
        location ~ ^/(login|register)$ {
            #	3.***************************************
            proxy_cache my_cache;   #   使用名为 my_cache 的缓存区
            proxy_cache_valid 200 302 10m;  #   对于状态为 200 和 302 的响应,缓存有效期为 10m
            #   设置通用的HTTP头部信息
            proxy_set_header Host $host; 
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            #   所有除了POST之外的所有请求
            limit_except POST {
                proxy_pass http://myapp;    #   对于    GET 请求,转发到myapp
            }
        }

        #   /login 和 /register的   POST    请求配置
        location ~ ^/(login|register)$ {
            proxy_pass http://myapp;
            #   设置通用的HTTP头部信息
            proxy_set_header Host $host; 
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }
    }
}
```

- `server`块定义了如何处理到来的客户端的请求,在`location`中,我们使用`proxy_pass`指令将请求转发到定义的上游服务器组
- `proxy_set header X-Forwarded-For $proxy_add_x_forwarded_for;`添加一个`X-Forwarded-For`头部,记录仪客户端请求经过的所有代理服务器的IP地址,`$proxy_add_x_forwarded_for`这个变量就会自动添加当前Nginx服务器的IP地址
- `proxy_set header X-Forwarded-Proto $scheme`传递原始请求使用的协议类型(http或https)给后续服务器,这对于后续服务器判断请求是否通过HTTPS加密非常重要

#### 出错的地方

1. 当用原来的Nginx服务时会发现

   ```bash
   [root@localhost http_WebServer]# ps aux | grep nginx
   root        3738  0.0  0.0  54952   876 ?        Ss   21:59   0:00 nginx: master process /usr/sbin/nginx
   nginx        3739  0.0  0.3  87672  6072 ?        S    21:59   0:00 nginx: worker process
   root        3740  0.0  0.2  87360  4912 ?        S    21:59   0:00 nginx: cache manager process
   ```

   此时直接访问`localhost:80`会没反应,查看nginx的错误日志(`/var/log/nginx/error.log`)可以发现:

   ```
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to 127.0.0.1:8080 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://127.0.0.1:8080/", host: "localhost"
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to [::1]:8080 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://[::1]:8080/", host: "localhost"
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to 127.0.0.1:8081 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://127.0.0.1:8081/", host: "localhost"
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to [::1]:8081 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://[::1]:8081/", host: "localhost"
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to 127.0.0.1:8082 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://127.0.0.1:8082/", host: "localhost"
   2024/07/14 21:26:41 [crit] 61103#0: *1 connect() to [::1]:8082 failed (13: Permission denied) while connecting to upstream, client: 127.0.0.1, server: , request: "GET / HTTP/1.1", upstream: "http://[::1]:8082/", host: "localhost"
   ```

   三次请求连接服务器都有问题,都提示`connect() to 127.0.0.1:8080 failed (13: Permission denied) while connecting to upstream`权限不够,此时一般有两种情况

   1. **访问的用户是`nginx`,无权限访问相应端口.**
      - 解决方法: 在`nginx.conf`最开始指定用户为`user root; `
   2. **linux自带的`SE Linux`(`Security-Enhanced Linux`)是Linux的一个内核模块,是一个安全子系统**
      - 解决方法: 关闭SE Linux

2. **vscode要自己配置转发端口,把端口打开**

   ![1720966533161](C:\Users\Luk1\Documents\WeChat Files\wxid_xyxyea2y6zxn12\FileStorage\Temp\1720966533161.png)

##### 配置`CMakeList.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyServerProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置app输出路径
set(HOME /home/luk1/code/http_WebServer/v7_nginx)   # 定义home路径
set(EXECUTABLE_OUTPUT_PATH ${HOME}/app) # 指定输出路径

-------------------------------------------------------------------------------------------------------------------
#   指定配置文件
set(NGINX_CONF "nginx.conf")
#   安装配置文件到目标位置
install(FILES ${NGINX_CONF}
        DESTINATION /etc/nginx
        COMPONENT config_files)
-------------------------------------------------------------------------------------------------------------------
# 添加可执行文件
add_executable(server main.cpp Database.hpp Logger.hpp ThreadPool.hpp HttpRequest.hpp HttpResponse.hpp HttpServer.hpp Router.hpp FileUtils.hpp)



# 外部库链接（如果有其他库需求，可以在此处添加）
target_link_libraries(server PRIVATE sqlite3 pthread)


# 编译选项
# 如果需要，可以添加其他编译选项
# target_compile_options(server PRIVATE ...)

# 如果需要链接其他库，可以使用 find_package 或 find_library 查找并链接它们


```

---

## 项目总结

### 基本架构

- **事件驱动模型**：采用`epoll`为I/O多路复用技术,实现非阻塞I/O操作,提高服务器处理并发连接的能力
- **线程池**：通过创建一定数量的线程并将它们放入线程池中,可以有效地管理复用线程资源,减少线程创建和线程的开销,提高服务器的响应速度
  - **线程安全**：使用互斥锁(例如`mutex`)保护临界资源,确保服务器在多线程环境下稳定运行
- **日志系统**：永宏函数方便的实现了Logger,便于记录服务器的运行信息

### 优势

- C++新特性
- 分布式反向代理
- SQL防注入

### 模型选择

本项目选用的**Reactor**模型

#### 弄清非阻塞同步模型和异步IO模型的区别

IO读取数据分为两个阶段，第一个阶段是内核准备好数据，第二个阶段是内核把数据从内核态拷贝到用户态。 

阻塞IO是当用户调用 read 后，用户线程会被阻塞，等内核数据准备好并且数据从内核缓冲区拷贝到用户态缓存区后， read 才会返回。阻塞IO是两个阶段都会阻塞，没有数据时也会阻塞。 

非阻塞IO是调用read后，如果没有数据就立马返回，通过不断轮询的方式去调用read，直到数据被拷贝到用户态的应用程序缓冲区，read请求才获取到结果。非阻塞IO阻塞的是第二个阶段，第一阶段没有数据时不会阻塞，第二阶段等待内核把数据从内核态拷贝到用户态的过程中才会阻塞。

同步 IO是应用程序发起一个 IO 操作后，必须等待内核把 IO 操作处理完成后才返回。无论 read 是阻塞 I/O，还是非阻塞 I/O， 都是同步调用，因为在 read 调用时，第二阶段内核将数据从内核空间拷贝到用户空间的过程都是需要等待的。 

异步 IO应用程序发起一个 IO 操作后，调用者不能立刻得到结果，而是在内核完成 IO 操作后，通过信号或回调来通知调用者。异步 I/O 是内核数据准备好和数据从内核态拷贝到用户态这两个过程都不用等待。 

总结一下，只有同步才有阻塞和非阻塞之分，异步必定是非阻塞的。 只有用户线程在操作IO的时候根本不去考虑IO的执行，全部都交给内核去完成， 而自己只等待一个完成信号的时候，才是真正的异步IO。select、poll、epool等IO多路复用方式都是同步的。
