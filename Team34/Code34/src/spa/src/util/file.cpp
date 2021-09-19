// file.cpp

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <string>
#include <string_view>

#include <zpr.h>

#include "util.h"

namespace util
{
    std::string readEntireFile(const char* path)
    {
        auto file = fopen(path, "rb");
        if(file == nullptr)
            util::error("misc", "failed to open file: {}", strerror(errno));

        std::string contents {};
        while(true)
        {
            char buf[1024] {};
            int n = 0;
            if(n = fread(buf, 1, 1024, file); n <= 0)
                break;

            contents += std::string_view(buf, n);
        }

        util::log("misc", "read input file '{}'", path);
        fclose(file);

        return contents;
    }

    FILE* getLogFile()
    {
        static struct _tmp
        {
            ~_tmp()
            {
                if(this->file)
                    fclose(this->file);
            }

            bool failed = false;
            FILE* file = nullptr;
        } file_handle;

        if(file_handle.file == nullptr)
        {
            if(file_handle.file = fopen(LOG_OUTPUT_FILE, "wb"); file_handle.file == nullptr)
            {
                file_handle.failed = true;
                fprintf(stderr, "failed to open log file! (%s)\n", strerror(errno));
            }

            if(file_handle.failed)
                return nullptr;
        }

        return file_handle.file;
    }
}
