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
    
    //  构造函数，初始化method和state
    HttpRequest(): method(UNKNOW),state(REQUEST_LINE){}
    
    //  一个POST请求示例
    /*
    POST /login HTTP/1.1                                -- parseRequestLine(line)
    HOST: localhost:8080                                -- parseHeader(line)    
    Content-Type: application/x-www-form-urlencoded     -- parseHeader(line)
    Contene-Length: 30                                  -- parseHeader(line)

    username=yuanshen&password=test1
    */

    //----------------------------------------
    //  后面凡是要解析的都将string变成了流对象，这样方便用getline来逐行解析
    //----------------------------------------

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

    //  获取请求路径的函数
    const string& getPath() const {
        return this->url;
    }

    //  其他成员函数和变量
    //  ...

private:

    //  此处是流对象因为要识别分隔符 ":"
    bool parseRequestLine(const string line) {
        istringstream iss(line);
        string method_str;
        iss >> method_str;
        if(method_str == "GET") method = GET;
        else if(method_str == "POST")  method = POST;
        else method = UNKNOW;

        iss >> url;         //  解析请求路径
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