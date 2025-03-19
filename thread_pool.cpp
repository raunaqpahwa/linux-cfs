//
// Created by Raunaq Pahwa on 3/4/25.
//

#include "thread_pool.h"

ThreadPool::ThreadPool(size_t num_threads): running_(true) {
    for (size_t i = 0; i < num_threads; i++) {
        workers_.emplace_back([this]() {
           while (running_) {
               std::unique_lock<std::mutex> lock(mutex_);
               available_.wait(lock, [this]() -> bool {
                   return not running_ or not tasks_.empty();
               });
               if (not running_) {
                   return;
               }
               auto task = std::move(tasks_.front());
               tasks_.pop();
               lock.unlock();
               task();
           }
        });
    }
}

ThreadPool::~ThreadPool() {
    running_ = false;
    available_.notify_all();
    for (auto& worker: workers_) {
        worker.join();
    }
}
