#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Add a job to run in the pool
    void enqueue(std::function<void()> job);

    // Stop accepting new jobs and finish queued jobs
    void shutdown();

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;

    std::mutex m_;
    std::condition_variable cv_;
    bool stopping_ = false;
};
