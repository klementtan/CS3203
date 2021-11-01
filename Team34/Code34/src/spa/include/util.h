// util.h

#pragma once

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <string>

#include <zpr.h>


// misc stuff
namespace util
{
    static constexpr const char* LOG_OUTPUT_FILE = "debug.log";

    std::string readEntireFile(const char* path);
    FILE* getLogFile();

#ifndef ENABLE_LOGGING
    static constexpr inline void dummy_fn() { }

#define logfmt(...) dummy_fn()

#else
    template <typename... Args>
    inline void logfmt(const char* who, const char* fmt, Args&&... args)
    {
        auto file = getLogFile();
        if(file == nullptr)
            return;

        zpr::fprintln(file, "[{}]: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
    }
#endif

    template <typename... Args>
    [[noreturn]] void error(const char* who, const char* fmt, const Args&... args)
    {
        // minor impl note: take the varargs by const& since we want to use them twice;
        // taking T&& would move them in the first call and UB the second call.

        // note: this prints to stderr, but we probably also want to print this to the log file.
        zpr::fprintln(stderr, "[{} ERROR]: {}", who, zpr::fwd(fmt, args...));
        logfmt(zpr::sprint("{} ERROR", who).c_str(), fmt, args...);

        exit(1);
    }

    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0814r0.pdf
    template <typename T>
    void _hash_combine(size_t& seed, const T& val)
    {
        seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <typename... Types>
    size_t hash_combine(const Types&... args)
    {
        size_t seed = 0;
        (_hash_combine(seed, args), ...);
        return seed;
    }
}

namespace zst
{
    using str_view = zpr::tt::str_view;
}
