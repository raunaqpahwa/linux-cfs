//
// Created by Raunaq Pahwa on 3/4/25.
//

#ifndef SCHEDULER_THREAD_POOL_H
#define SCHEDULER_THREAD_POOL_H

#include <cstddef>
#include <future>
#include "mutex"
#include "queue"
#include "thread"
#include "functional"

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);

    template <typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        std::unique_lock<std::mutex> lock(mutex_);
        if (not running_) throw std::runtime_error("Thread pool is not running");
        tasks_.emplace([task]() { (*task)(); });
        available_.notify_one();
        return result;
    }

    ~ThreadPool();
private:
    std::atomic<bool> running_;
    std::mutex mutex_;
    std::condition_variable available_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
};
#endif //SCHEDULER_THREAD_POOL_H
