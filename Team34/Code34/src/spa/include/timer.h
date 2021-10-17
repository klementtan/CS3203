#include <chrono>
#include <string>
#include <zpr.h>
#include <util.h>
#include <string.h>


namespace bench
{
    struct Timer
    {
        std::string fn, title;
        std::chrono::time_point<std::chrono::steady_clock> start;
        Timer(std::string fn, std::string title)
            : fn(std::move(fn)), title(std::move(title)), start(std::chrono::steady_clock::now())
        {
        }
        ~Timer()
        {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
            util::logfmt("TIMER", "{}: function={}; elapsed={}ms", title, fn, elapsed / 1000.0);
        }
    };
}

#ifndef ENABLE_BENCHMARK
static constexpr inline void dummy_fn() { }
#define START_BENCHMARK_TIMER(...) dummy_fn()
#else

#define START_BENCHMARK_TIMER(title) bench::Timer timer(__FUNCTION__, title)
#endif
