// misc.cpp

#include <zst.h>
#include "util.h"

namespace zst
{
    [[noreturn]] void error_and_exit(const char* msg, size_t n)
    {
        util::error("???", "{}", zst::str_view(msg, n));
    }
}
