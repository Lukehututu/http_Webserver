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