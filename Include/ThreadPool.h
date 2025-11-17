#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace spqr {

class ThreadPool {
   public:
    explicit ThreadPool(size_t numThreads) : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }

        condition_.notify_all();

        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Delete copy/move constructors and assignment operators
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Enqueue a task and return a future for the result
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            if (stop_) {
                throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }

        condition_.notify_one();
        return result;
    }

    // Wait for all currently enqueued tasks to complete
    void waitAll() {
        std::vector<std::future<void>> futures;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            // Snapshot current task count
            size_t taskCount = tasks_.size();

            // Create dummy tasks that we can wait on
            for (size_t i = 0; i < taskCount; ++i) {
                auto task = std::make_shared<std::packaged_task<void()>>([]() {});
                futures.push_back(task->get_future());
                tasks_.emplace([task]() { (*task)(); });
            }
        }

        condition_.notify_all();

        // Wait for all futures
        for (auto& future : futures) {
            future.wait();
        }
    }

   private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_;
};

}  // namespace spqr
