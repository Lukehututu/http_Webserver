#include <iostream>         
#include <functional>           //  引入可调用对象包装器         
#include <thread>               //  引入c11线程的头文件
#include <vector>               //  用vector来管理线程队列
#include <condition_variable>   //  引入条件变量，用于线程阻塞和唤醒
#include <unistd.h>             //  引入
#include "TaskQueue.hpp"        //  引入任务队列的头文件

class ThreadPool {
public:


    ThreadPool(int min,int max) {
        //  初始化任务队列
        m_taskQ = new TaskQueue;

        //  初始化其他对象
        m_maxNum = max;
        m_minNum = min;
        m_busyNum = 0;
        m_shutdown = false;

        //  根据max来给vector预留内存
        m_threads.resize(max * sizeof(thread));

        //////////////////////  创建线程    //////////////////////
        //  根据min来创建线程
        for(int i = 0;i < min;++i) {
            m_threads.emplace_back(worker(this));
        }
        //  创建管理者线程
        m_managerThread = thread(manager(this));

    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(this->m_mutex);
            this->m_shutdown = true;
        }
        //  先修改m_shutdown再唤醒所有线程让他们自杀
        condition.notify_all();
        //  等待所有工作线程自杀然后再自杀
        for(thread& trd : this->m_threads) {
            if(trd.joinable()) {
                trd.join();
            }
        }
        this->m_threads.clear();
    }
    
    //  添加任务
    void addTask(Task task) {
        if(this->m_shutdown) {
            return;
        }
        //  添加任务，但不需要加锁，因为taskQ的方法中有锁了
        this->m_taskQ->addTask(task);
        //  唤醒工作线程
        unique_lock<mutex> lock(this->m_mutex);
        condition.notify_one();
    }

    //  获取在忙线程个数
    int getBusyNum() {
        int bn = 0;
        unique_lock<mutex> lock(this->m_mutex);
        bn = this->m_busyNum;
        return bn;
    }

    //  获取活着的线程个数
    int getAliveNum() {
        int an = 0;
        unique_lock<mutex> lock(this->m_mutex);
        an = this->m_aliveNum;
        return an;
    }

private:
    //  工作线程的任务函数
    static void* worker(void* arg) {
        //  先获取pool的指针 通过static_cast强转(cpp风格的强转)
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        //  一直工作
        while(1) {
            Task task;
            //  中间有很长一段逻辑要访问m_taskQ因此要加锁
            {
                //  因为要访问任务队列，因此要加锁
                unique_lock<mutex> lock(pool->m_mutex);
                //  判断任务队列状态,如果为0且线程池还在工作那就阻塞
                condition.wait(lock,[pool] {return pool->m_shutdown || pool->m_taskQ->taskNumber() == 0;});
                //  解除阻塞，要么是线程池关闭要么是来任务了或者是管理者认为线程太多了要线程结束
                if(pool->m_exitNum > 0) {
                    pool->m_aliveNum--;
                    //  不用解锁因为通过下面的函数该线程直接噶了lock销毁锁就自动解开了
                    pool->threadExit();
                }
                if(pool->m_shutdown) {
                    pool->m_aliveNum--;
                    //  不用解锁因为通过下面的函数该线程直接噶了lock销毁锁就自动解开了
                    pool->threadExit();
                }
                //  取出任务
                task = pool->m_taskQ->takeTask();
                pool->m_busyNum++;
            }
            //  当前线程执行任务
            task.function(task.arg);
            delete task.arg;
            task.arg = nullptr;

            //  任务处理结束
            cout<<"thread "<<this_thread::get_id()<<" end working...\n";
            pool->m_mutex.lock();
            pool->m_busyNum--;
            pool->m_mutex.lock();
        }
        return nullptr;
    }
    
    //  管理者线程的任务函数    --  主要负责线程监控和调度
    static void* manager(void* arg) {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        
        //  只要线程池没有关闭就一直检测
        while(!pool->m_shutdown) {
            
            //  每隔5s检测一次
            sleep(5);
            //  取出线程池中的任务数量和线程数量
            int queue_sz,livenum,busynum;
            //  要访问taskQ因此要锁
            {
                unique_lock<mutex> lock(pool->m_mutex);
                queue_sz = pool->m_taskQ->taskNumber();
                livenum = pool->getAliveNum();
                busynum = pool->getBusyNum();
            }

            //  创建线程
            const int NUM = 2;
            //  如果当活着的线程数少于任务数量 && 同时当前活着线程数少于最大线程数量
            if(livenum < queue_sz && livenum < pool->m_maxNum) {
                //  因为要访问线程池因此要加锁
                {
                    unique_lock<mutex> lock(pool->m_mutex);


                }
            }

            //  销毁多余的线程
            //  忙线程*2 <  存活的线程数 && 存活的线程数 > 最小的线程数量
            if(pool->m_busyNum*2 < pool->m_aliveNum && pool->m_aliveNum > pool->m_minNum) {
                {
                    unique_lock<mutex> lock(pool->m_mutex);
                    pool->m_exitNum = NUM;
                }
                for(int i = 0;i < NUM;++i) {
                    condition.notify_one();
                }
            }
        }
        return nullptr;
    }

    //  线程退出
    void threadExit() {
        std::thread::id this_id = std::this_thread::get_id();

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
            if (it->get_id() == this_id) {
                std::cout << "threadExit() function: thread " << this_id << " exiting..." << std::endl;
                it->detach();
                m_threads.erase(it); 
                break;
            }
        }
    }


private:
    TaskQueue* m_taskQ;                 //  一个任务队列

    mutex m_mutex;                      //  互斥锁
    vector<thread> m_threads;           //  线程队列
    thread m_managerThread;             //  管理者线程   
    static condition_variable condition;       //  条件变量用来阻塞和唤醒线程  
    int m_maxNum;                       //  最大线程数
    int m_minNum;                       //  最小线程数
    int m_busyNum;                      //  在忙的线程数
    int m_aliveNum;                     //  还活着的线程数
    int m_exitNum;                      //  要销毁的线程数
  
    bool m_shutdown;                    //  用于控制线程池的声明周期
};