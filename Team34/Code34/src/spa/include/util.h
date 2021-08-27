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

    template <typename... Args>
    void log(const char* who, const char* fmt, Args&&... args)
    {
        static struct _tmp
        {
            ~_tmp()
            {
                if(this->file)
                    fclose(this->file);
            }

            FILE* file = nullptr;
        } file_handle;

        if(file_handle.file == nullptr)
        {
            if(file_handle.file = fopen(LOG_OUTPUT_FILE, "wb"); file_handle.file == nullptr)
            {
                fprintf(stderr, "failed to open log file! (%s)\n", strerror(errno));
                return;
            }
        }

        zpr::fprintln(file_handle.file, "[{}]: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
    }
}
