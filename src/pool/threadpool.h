#pragma once

#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <future>
#include <memory>
#include <stdexcept>

const int THREAD_POOL_DEFAULT_SIZE = 4;

using JobHandler = std::function<void()>;

class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<JobHandler> jobs_;

    std::mutex mutex_;
    std::condition_variable condition_;

    bool is_stop_ = false;

public:
    ThreadPool(int pool_size = THREAD_POOL_DEFAULT_SIZE) {
        workers_.reserve(pool_size);

        for (int i = 0; i < pool_size; ++i) {
            workers_.emplace_back([this]{
                // 从任务队列中取出任务
                JobHandler job{};
                while (!is_stop_) {
                    {
                        std::unique_lock<std::mutex> lock(mutex_);

                        condition_.wait(lock, [this]{
                            return is_stop_ || !jobs_.empty();
                        });

                        if (!jobs_.empty()) {
                            job = std::move(jobs_.front());
                            jobs_.pop();
                        } else {
                            break;
                        }
                    }

                    // 执行任务
                    job();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_stop_ = true;
        }
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

public:
    template<class F, typename ...Args>
    auto enqueue(F&& f, Args&& ...args) -> std::future<std::result_of_t<F(Args...)>>  {
        using return_type = std::result_of_t<F(Args...)>;

        // 构造一个异步任务
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> fut = task->get_future();

        // 将任务添加到任务队列中
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (is_stop_) {
                throw "enqueue in thread pool stoped.";
            }

            // 值引用，此处是shared_ptr, 会保留一个副本
            jobs_.emplace([task]{
                (*task)();
            });
        }

        // 通知消费者消费
        condition_.notify_one();


        return fut;
    }
};