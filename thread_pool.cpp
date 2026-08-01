#include "thread_pool.h"

ThreadPool::ThreadPool(int cntThread) : stop(false){
    if (cntThread == 0){
        cntThread = 4;
    }

    for (int i = 0; i < cntThread; i++){
        workers.emplace_back([this]{
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queueMutex);
                this->cv.wait(lock, [this]{return this->stop || !this->tasks.empty();});
                if (this->stop && this->tasks.empty()){
                    return;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        });
    }
}

void ThreadPool::enqueue(std::function<void()> task){
    std::unique_lock<std::mutex> lock(queueMutex);
    if (stop) return;
    tasks.push(std::move(task));
    cv.notify_one();
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    cv.notify_all();
    for (size_t i = 0; i < workers.size(); i++){
        if (workers[i].joinable()){
            workers[i].join();
        }
    }
}
