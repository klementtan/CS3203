// util.h

#pragma once

#include <cstdio>
#include <cerrno>
#include <cstring>

#include <string>

#include <zpr.h>
#include "simple/ast.h"

// misc stuff
namespace util
{
    namespace s_ast = simple::ast;

    static constexpr const char* LOG_OUTPUT_FILE = "debug.log";

    std::string readEntireFile(const char* path);
    FILE* getLogFile();

    template <typename... Args>
    void log(const char* who, const char* fmt, Args&&... args)
    {
        auto file = getLogFile();
        if(file == nullptr)
            return;

        zpr::fprintln(file, "[{}]: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
    }

    template <typename... Args>
    [[noreturn]] void error(const char* who, const char* fmt, const Args&... args)
    {
        // minor impl note: take the varargs by const& since we want to use them twice;
        // taking T&& would move them in the first call and UB the second call.

        // note: this prints to stderr, but we probably also want to print this to the log file.
        zpr::fprintln(stderr, "[{} ERROR]: {}", who, zpr::fwd(fmt, args...));
        log(zpr::sprint("{} ERROR", who).c_str(), fmt, args...);

        exit(1);
    }
}
