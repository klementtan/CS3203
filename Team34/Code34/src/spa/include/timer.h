#include <iostream>
#include <chrono>
#include <string>

namespace bench
{
  struct Timer {
    std::string file;
    int line;
    std::string fn, title;
    std::chrono::time_point<std::chrono::steady_clock> start;
    Timer(std::string file, int line, std::string fn, std::string title)
      : file(file),
        line(line),
        fn(fn),
        title(title),
        start(std::chrono::steady_clock::now())
    {}
    ~Timer() {
      const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
      std::string where = file + std::string(":") + std::to_string(line);
      std::string text = "[TIMER: " + title + "];"
                          + "function=" + fn + ";"
                          + "where=" + where + ";"
                          + "elapsed=" + std::to_string(elapsed) + "ms";
      std::cout << text << "\n";
    }
  };
}

#define START_BENCH_TIMER(title) \
        bench::Timer timer(__FILE__, __LINE__, __FUNCTION__, title)


