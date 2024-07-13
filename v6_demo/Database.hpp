#pragma once
#include <sqlite3.h>        //  数据库的头文件
#include <string>
#include <stdexcept>        //  错误管理的头文件

#include "Logger.hpp"
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
	    sqlite3_finalize(stmt);
        if(password_str.compare(password) != 0) {
            //  如果密码不匹配，记录日志并返回false
            LOG_INFO("Login failed for user: %s",username.c_str());
            return false;
        }
        //  登录成功，记录日志并返回true
        LOG_INFO("User logged in: %s",username.c_str());
        return true;
    }

};