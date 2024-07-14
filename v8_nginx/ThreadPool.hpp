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