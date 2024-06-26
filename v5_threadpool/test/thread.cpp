#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"
using namespace std;


int main() {
    ThreadPool pool(4);
    vector<future<int>> results;

    for(int i = 0;i < 8;i++) {
        results.emplace_back(
            pool.enqueue([i]{
                cout<<"hello "<<i<<endl;
                this_thread::sleep_for(chrono::seconds(1));
                cout<<"world "<<i<<endl;
                return i*i;
            })
        );
    }
    pool.~ThreadPool();
    for(auto && res : results) {
        cout<<res.get()<<" ";
    }
    cout<<"\n";
}