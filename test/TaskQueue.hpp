#pragma once
#include <queue>
#include <mutex>
using namespace std;

using callback = void(*)(void* arg);

//  任务结构体
struct Task {
    Task() {
        function = nullptr;
        arg = nullptr;
    }

    Task(callback f,void* arg) {
        this->function = f;
        this->arg = arg;
    }

    callback function;
    void* arg;
};


/*  任务队列
    提供：
        1.  添加任务
        2.  取出任务
        3.  存储任务
        4.  获取任务个数
        5.  线程同步
            ... 等功能  */  
class TaskQueue {
public:

    TaskQueue() {
        
    }

    ~TaskQueue() {

    }
    //  因为任务队列是临界资源，所以在访问时需要锁住

    //  添加任务    --  要么已经有了现成的任务
    void addTask(Task& task) {
        //  手动锁定与解锁
        m_mutex.lock();
        m_taskQueue.push(task);
        m_mutex.unlock();

        //  自动锁定与解锁
        //  此时创建了一个lock_guard<mutex>对象 此时调用其构造函数同时锁定m_mutex
        /*
        lock_guard<mutex> lock(&m_mutex);
        ...
        */
        //  在函数结尾该对象自动销毁于是调用析构函数，同时解锁m_mutex
    }


    //              -- 要么没有现成任务那就传进两个成员直接构造一个
    void addTask(callback func,void* arg) {
        
        Task task(func,arg);
        m_mutex.lock();
        m_taskQueue.push(task);
        m_mutex.unlock();
    }

    //  取出任务
    Task takeTask() {
        Task task;
        
        if(!m_taskQueue.empty()) {

            lock_guard<mutex> lock(m_mutex);
            task = m_taskQueue.front();
            m_taskQueue.pop();
        
        }
        
        return task;
    }

    //  获取当前队列中任务的个数    --  如果只有几行没有循环和判断的func可以直接内联，这样效率高
    inline int taskNumber() {
        return m_taskQueue.size();
    }


private:
    queue<Task> m_taskQueue;    //  任务队列
    mutex   m_mutex;        //  互斥锁

};
