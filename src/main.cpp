#include "thread/threadpool.h"
#include "minilog/minilog.h"
#include <thread>

int main() {
    for (int i = 0; i < 100; i++) {
        ThreadPool::instance().commit(
            [&](int a) {
                minilog::log_info(
                    "num: {} ,thread {}", a,
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
            },
            i);
    }
    minilog::log_info("done");
    return 0;
}