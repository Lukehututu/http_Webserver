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

最需要注意的是当要接收客户端请求的时候read返回值的处理

```cpp
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

