#pragma once
#include <mysql/mysql.h>        //  数据库的头文件
#include <string>
#include <stdexcept>        //  错误管理的头文件
#include <cstring>        

#include "Logger.hpp"
using namespace std;


class Database {
private:
    MYSQL mysql;
    MYSQL *conn;

public:
    //  构造函数，用于打开数据库并创建用户表
    Database() {
       conn = mysql_init(&mysql);
       if(conn == nullptr) {
            LOG_ERROR("Failed to initialize MySQL connection");
       } 
       conn = mysql_real_connect(conn,"localhost","root","1234","webserver",0,NULL,0);
       if(conn == nullptr) {
            LOG_ERROR("Failed to connect to MySQL server");
            mysql_close(conn);
            throw std::runtime_error("Failed to connect to MySQL server");
       }
       LOG_INFO("Connected to MySQL server");
       //  创建用户表
       string create_table_sql = 
                                    "CREATE TABLE IF NOT EXISTS users("                               
                                    "username VARCHAR(15) PRIMARY KEY, "
                                    "password VARCHAR(10)"
                                    ")";
        if(mysql_query(conn,create_table_sql.c_str())){
            fprintf(stderr, "%s\n", mysql_error(conn));
        } else {
            LOG_INFO("Created users table");
        }
        
    }

    //  析构函数，用于关闭数据库连接
    ~Database() {
        conn = nullptr;
        mysql_close(&mysql);
    }

    //  用户注册函数    -- 需要接收前端传入的username 和 password
    bool registerUser(const string& username, const string& password) {
        //  预先构建sql语句，用于插入新用户
        string  insert_sql = "INSERT INTO users (username,password) values(?,?)";
        //  准备预处理对象  --  直接将创建和初始化预处理对象结合
        MYSQL_STMT *stmt = mysql_stmt_init(conn);
        if(stmt == nullptr) {
            LOG_ERROR("Failed to initialize MySQL statement");
            return false;
        }
        //  绑定字符串操作指定的sql语句
        if(mysql_stmt_prepare(stmt,insert_sql.c_str(),insert_sql.length()) != 0) {
            LOG_ERROR("Failed to prepare MySQL statement");
            mysql_stmt_close(stmt);
            return false;
        }
        //  绑定参数，即替换 ？ 
        MYSQL_BIND params[2];
        //  就是因为没有初始化
        memset(params,0,sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer_length = username.length();
        params[0].buffer = (char*)username.c_str();

        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = (char*)password.c_str();
        params[1].buffer_length = password.length();


        //  此时已经填充了params
        //  开始绑定参数到？
        if(mysql_stmt_bind_param(stmt,params) != 0) {
            LOG_ERROR("Failed to bind parameters");
            mysql_stmt_close(stmt);
            return false;
        }

        //  开始执行    该函数成功返回0，失败返回非0
        if(mysql_stmt_execute(stmt) != 0) {
            int errorNumber = mysql_stmt_errno(stmt);
            const char* errorMessage = mysql_stmt_error(stmt);
            LOG_ERROR("Failed to execute statement: %s (Error %d)", errorMessage, errorNumber);
            mysql_stmt_close(stmt);
            return false;
        }

        //  关闭预处理句柄
        mysql_stmt_close(stmt);
        //  一般不记录敏感信息，但此时是为了调试
        LOG_INFO("Registered user %s password %s",username.c_str(),password.c_str());
        return true;
    }

    //  用户登录函数
    bool loginUser(const string& username,const string& password) {
        //  构建查询用户密码的sql语句
        string sql = "SELECT password FROM users WHERE username = ?";
        //  准备stmt
        MYSQL_STMT * stmt = mysql_stmt_init(conn);
        if(stmt == nullptr) {
            LOG_ERROR("Failed to initialized MySQL statement");
            return false;
        }

        if(mysql_stmt_prepare(stmt,sql.c_str(),sql.length()) != 0 ) {
            LOG_ERROR("Failed to prepare MySQL statement");
            mysql_stmt_close(stmt);
            return false;
        }

        MYSQL_BIND param;
        memset(&param,0,sizeof(param));
        param.buffer_type = MYSQL_TYPE_STRING;
        param.buffer_length = username.length();
        param.buffer = (char*)username.c_str();

        //  如果执行的是查询语句，那就是用bind_param
        if(mysql_stmt_bind_param(stmt,&param) != 0) {
            LOG_ERROR("Failed to bind MySQL statement");
            mysql_stmt_close(stmt);
            return false;
        }

        //  执行
        if(mysql_stmt_execute(stmt) != 0) {
            LOG_ERROR("Failed to exec MySQL statement");
            mysql_stmt_close(stmt);
            return false;
        }


        //  准备结果集
        MYSQL_BIND result;
        memset(&result,0,sizeof(result));
        result.buffer_type = MYSQL_TYPE_STRING;
        result.buffer_length = 256;
        result.buffer = new char[256];
        result.length = &result.buffer_length;
        //  绑定结果集
        if(mysql_stmt_bind_result(stmt,&result) != 0) {
            LOG_ERROR("Failed to bind result");
            delete[] result.buffer;
            mysql_stmt_close(stmt);
            return false;
        }

        //  获取结果
        if(mysql_stmt_fetch(stmt) == 0) {
            //  现在ret_ps包含了查询到的密码
            string ret_ps = string((char*)result.buffer,result.buffer_length);
            if(password.compare(ret_ps) != 0) {
                LOG_ERROR("Login failed for user %s",username.c_str());
                delete[] result.buffer;
                mysql_stmt_close(stmt);
                return false;
            }
        } else {
            int errnum = mysql_stmt_errno(stmt);
            const char* errmsg = mysql_stmt_error(stmt);
            LOG_ERROR("Failed to fetch data: Error %d: %s", errnum, errmsg);
            mysql_stmt_close(stmt);
            delete[] result.buffer;
            return false;
        }

        delete[] result.buffer;
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        LOG_INFO("User %s login ",username.c_str());
        return true;
    }

};