#include <doctest/doctest.h>
#include <nanobench.h>

#include "thread/threadpool.h"
#include "minilog/minilog.h"
#include <thread>

void test(int k) {
    for (int i = 0; i < 10000; i++) {
        ThreadPool::instance().commit(
            [](int a) {
                minilog::log_info(
                    "num: {} ,thread {}", a,
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
            },
            i);
    }
    return;
}

// NOLINTNEXTLINE
TEST_CASE("thread-pool") {
    minilog::set_log_level(minilog::log_level::warn);
    ankerl::nanobench::Bench().run("++x", [&]() {
        for (int i = 0; i < 10001; i++) {
            test(i);
            if (i % 100 == 0) { minilog::log_warn("done{}", i); }
        }
    });
}
