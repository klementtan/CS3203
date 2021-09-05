#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include <zst.h>
#include <zpr.h>
#include "simple_parser.h"
#include "pkb.h"

using namespace simple_parser;
using namespace pkb;

static void req(bool b)
{
    REQUIRE(b);
}

static std::string get_content(const char* path, const char* mode)
{
    FILE* file = stdin;
    file = fopen(path, mode);
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

        contents += zst::str_view(buf, n).sv();
    }
    return contents;
}


TEST_CASE("Parse program")
{
    SECTION("Multiple assignments")
    {
        std::string in = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\recursive_calls.txt", "rb");

        auto prog = parseProgram(in);
        auto pkb = processProgram(prog);
        req(true);
    }
}