// file.cpp

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <string>
#include <string_view>

#include <zpr.h>

namespace frontend
{
    std::string readEntireFile(const char* path)
    {
        auto file = fopen(path, "rb");
        if(file == nullptr)
        {
            zpr::fprintln(stderr, "failed to open file: {}", strerror(errno));
            exit(1);
        }

        std::string contents {};
        while(true)
        {
            char buf[1024] {};
            int n = 0;
            if(n = fread(buf, 1, 1024, file); n <= 0)
                break;

            contents += std::string_view(buf, n);
        }

        return contents;
    }
}
