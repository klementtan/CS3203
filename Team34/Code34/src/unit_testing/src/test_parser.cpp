// test_parser.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include <zst.h>
#include <zpr.h>

#include "ast.h"
#include "simple_parser.h"
#include "frontend/parser.cpp"

static void req(bool b)
{
    REQUIRE(b);
}

using namespace simple_parser;
using namespace ast;

static bool check_expr(zst::str_view sv, zst::str_view expect, Expr* (*func)(ParserState*) )
{
    auto ps = ParserState { sv };
    auto res = func(&ps);
    if(res->toString() == expect)
    {
        return true;
    }
    else
    {
        zpr::fprintln(stderr, "Invalid test case\ngiven sv: {}, given expectation: {}, obtained res: {}"
                        , sv, expect, res->toString());
        return false;
    }
}

static std::string get_content(const char* path, const char* mode) {
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

// start from the bottom
TEST_CASE("Parse expr")
{
    req(true);
}


TEST_CASE("Parse primary")
{
    SECTION("Constant") {
        req(check_expr("2", "2", &parsePrimary));
    }
    SECTION("Var") {
        req(check_expr("a", "a", &parsePrimary));
    }
    SECTION("Expr in parenthesis")
    {
        req(check_expr("(a)", "a", &parsePrimary));
    }
}

TEST_CASE("Parse rhs")
{
    req(true);
}

TEST_CASE("Parse conditional expr")
{
    SECTION("!")
    {
        req(true);
    }
    SECTION("(")
    {
        req(true);
    }
    SECTION("Expr")
    {
        req(true);
    }
}
TEST_CASE("Parse stmt list")
{
    req(true);
}
TEST_CASE("Parse stmt")
{
    req(true);
}
TEST_CASE("Parse if statement")
{
    req(true);
}
TEST_CASE("Parse while stmt")
{
    req(true);
}
TEST_CASE("Parse procedure") {
    req(true);
}
TEST_CASE("Parse program")
{
    SECTION("Multiple assignments") 
    {
        std::string in = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\src1.txt", "rb");
        std::string out = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\out1.txt", "r");

        auto prog = parseProgram(in);
        req(prog->toProgFormat().compare(out) == 0);
    }
    SECTION("Bad formatting")
    {
        std::string in = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\src2.txt", "rb");
        std::string out = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\out2.txt", "r");

        auto prog = parseProgram(in);
        req(prog->toProgFormat().compare(out) == 0);
    }
    SECTION("Conditional stmts and while") {
        std::string in = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\src3.txt", "rb");
        std::string out = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\out3.txt", "r");
        
        auto prog = parseProgram(in);
        zpr::fprintln(stdout, "begin{}end", prog->toProgFormat());
        req(true);
    }
    SECTION("Conditional stmts and while")
    {
        std::string in = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\src4.txt", "rb");
        std::string out = get_content("..\\..\\..\\..\\src\\unit_testing\\simple_prog\\out4.txt", "r");

        auto prog = parseProgram(in);
        zpr::fprintln(stdout, "begin{}end", prog->toProgFormat());
        req(true);
    }
}
