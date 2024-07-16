//  该类主要根据请求的路径和方法，决定调用哪个处理函数，实现请求的具体逻辑
#pragma once
#include <functional>
#include <string>
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
                response.setStatusCode(302);
                response.setHeader("Location","/index.html"); 
                return response;
            } else {
                response.setStatusCode(401);    //  401--未授权 Unauthorized
                response.setBody("<html><body><h2>Login Failed</h2></body></html>");
                return response;
            }
        });
    }

    //  通过传进来的request来分配处理函数
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
    
    unordered_map<string,RequestHandler> routes;    //  存储路由映射
};