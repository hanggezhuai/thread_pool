#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <vector>
#include <future>
#include <iostream>

class CantCopy {
  public:
    ~CantCopy() {}

  protected:
    CantCopy() {};

  private:
    CantCopy(const CantCopy &) = delete;
    CantCopy &operator=(const CantCopy &) = delete;
};

class ThreadPool : public CantCopy {
  public:
    using Task = std::packaged_task<void()>;
    ~ThreadPool() {
        /*is_task_empty_.notify_all();
        std::unique_lock<std::mutex> is_task(m);
        is_task_empty_.wait(is_task, [&]() { return tasks_.empty(); });
        */
        while (!tasks_.empty());
        stop();
    }

    static ThreadPool &instance() {
        static ThreadPool ins;
        return ins;
    }

    template<class F, class... Args>
    auto commit(F &&f, Args &&...args) -> std::future<decltype(std::forward<F>(
                                           f)(std::forward<Args>(args)...))> {
        using ReType =
            decltype(std::forward<F>(f)(std::forward<Args>(args)...));
        if (stop_.load()) { return std::future<ReType>{}; }
        auto task = std::make_shared<std::packaged_task<ReType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task] { (*task)(); });
        }
        cv_lock_.notify_one();
        return ret;
    }

    int idleThreadCount() { return thread_num_; }

  private:
    std::atomic_int thread_num_;
    std::queue<Task> tasks_;
    std::vector<std::thread> pool_;
    std::atomic_bool stop_;
    std::mutex cv_mt_;
    std::condition_variable cv_lock_; //, is_task_empty_;

    void strat() {
        for (int i = 0; i < thread_num_; ++i) {
            pool_.emplace_back([this]() {
                while (!this->stop_.load()) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> cv_mt(cv_mt_);
                        this->cv_lock_.wait(cv_mt, [this] {
                            return this->stop_.load() || !this->tasks_.empty();
                        });
                        if (this->tasks_.empty()) { return; }
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    this->thread_num_--;
                    task();
                    this->thread_num_++;
                }
            });
        }
    }

    void stop() {
        stop_.store(true);
        cv_lock_.notify_all();
        for (auto &td : pool_) {
            if (td.joinable()) {
                std::cout << "kill thread "
                          << std::hash<std::thread::id>{}(td.get_id())
                          << std::endl;
                td.join();
            }
        }
    }

    ThreadPool(unsigned int num = std::thread::hardware_concurrency())
        : stop_(false) {
        if (num <= 1) {
            thread_num_ = 2;
        } else {
            thread_num_ = num < 5 ? num : 5;
        }
        strat();
    }
};