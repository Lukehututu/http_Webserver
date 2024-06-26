#include <iostream>             // 包含输入输出流的头文件
#include <thread>               // 包含线程库的头文件
#include <mutex>                // 包含互斥锁的头文件
#include <condition_variable>   // 包含条件变量的头文件
using namespace std;
std::mutex mtx;                 // 定义互斥锁对象 mtx
std::condition_variable cv;     // 定义条件变量对象 cv
bool ready = false;             // 定义共享变量 ready

void worker() {
    cout<<"this is worker sleeping...\n";
    {   
        unique_lock<mutex> lock(mtx);
        cv.wait(lock,[]{ return ready; });
    }
    cout<<"this is worker ...\n";

}

int main() {

    thread wk(worker);
    cout<<"this is main...\n";
    this_thread::sleep_for(chrono::seconds(1));
    {
        unique_lock<mutex> lock(mtx);
        ready = true;
    }
    cv.notify_one();
    wk.join();
    cout<<"exit ...\n";
}