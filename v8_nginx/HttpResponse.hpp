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
    void setStatusCode(int code) {
        this->statusCode = code;
    }

    //  设置响应体
    void setBody(const string& body) {
        this->body = body;
    }

    //  设置响应头
    void setHeader(const string name,const string value) {
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
        response.setHeader("Content-Type","text/plain");
        return response;
    }
    
    //  生成正确请求
    static HttpResponse makeOKResponse(const string sta) {

    }


private:
    
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