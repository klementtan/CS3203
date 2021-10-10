#include <chrono>
#include <string>
#include <zpr.h>

namespace bench
{
    struct Timer
    {
        std::string file;
        int line;
        std::string fn, title;
        std::chrono::time_point<std::chrono::steady_clock> start;
        Timer(std::string file, int line, std::string fn, std::string title)
            : file(std::move(file)), line(std::move(line)), fn(std::move(fn)), title(std::move(title)),
              start(std::chrono::steady_clock::now())
        {
        }
        ~Timer()
        {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
            zpr::println("[TIMER: {}]: function={}; where={}:{}; elapsed={}ms", title, fn, file, line, elapsed);
        }
    };
}

#ifndef ENABLE_BENCHMARK
static constexpr inline void dummy_fn() { }
#define START_BENCHMARK_TIMER(...) dummy_fn()
#else
#define START_BENCHMARK_TIMER(title) bench::Timer timer(__FILE__, __LINE__, __FUNCTION__, title)
#endif
