// test_calls

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

// clang-format off
constexpr const auto src = R"(
    procedure A { call B; }
    procedure B { call C; call D; call E; }
    procedure C { if (1 == 2) then { call D; } else { call E; } }
    procedure D { print X; }
    procedure E { call F; }
    procedure F { call G; call H; }
    procedure G { call X; }
    procedure H { call P; }
    procedure X { print A; }
    procedure P { print P; }
    procedure Z { print Z; }
    procedure AA { read x; }
    procedure AB { read x; }
)";

TEST_CASE("Calls(EntRef, EntRef)")
{
    TEST_OK(src, "print p; Select p such that Calls(\"A\", \"B\")", 8, 14, 15, 16);
    TEST_OK(src, "print p; Select p such that Calls(\"G\", \"X\")", 8, 14, 15, 16);
    TEST_EMPTY(src, "print p; Select p such that Calls(\"A\", \"E\")");
}

TEST_CASE("Calls(EntRef, Decl)")
{
    TEST_OK(src, "procedure p; Select p such that Calls(\"B\", p)", "C", "D", "E");
    TEST_OK(src, "procedure p; Select p such that Calls(\"F\", p)", "G", "H");
    TEST_EMPTY(src, "procedure p; Select p such that Calls(\"D\", p)");
}

TEST_CASE("Calls(EntRef, _)")
{
    TEST_OK(src, "print p; Select p such that Calls(\"A\", _)", 8, 14, 15, 16);
    TEST_EMPTY(src, "print p; Select p such that Calls(\"D\", _)");
}

TEST_CASE("Calls(Decl, EntRef)")
{
    TEST_OK(src, "procedure p; Select p such that Calls(p, \"B\")", "A");
    TEST_OK(src, "procedure p; Select p such that Calls(p, \"D\")", "B", "C");
    TEST_OK(src, "procedure p; Select p such that Calls(p, \"E\")", "B", "C");
    TEST_EMPTY(src, "procedure p; Select p such that Calls(p, \"A\")");
    TEST_EMPTY(src, "procedure p; Select p such that Calls(p, \"Z\")");
}

TEST_CASE("Calls(Decl, Decl)")
{
    TEST_OK(src, "procedure p, q; Select p such that Calls(p, q)", "A", "B", "C", "E", "F", "G", "H");
    TEST_OK(src, "procedure p, q; Select q such that Calls(p, q)", "B", "C", "D", "E", "F", "G", "H", "X", "P");
}

TEST_CASE("Calls(Decl, _)")
{
    TEST_OK(src, "procedure p; Select p such that Calls(p, _)", "A", "B", "C", "E", "F", "G", "H");
}

TEST_CASE("Calls(_, EntRef)")
{
    TEST_OK(src, "print p; Select p such that Calls(_, \"D\")", 8, 14, 15, 16);
    TEST_EMPTY(src, "print p; Select p such that Calls(_, \"A\")");
}

TEST_CASE("Calls(_, Decl)")
{
    TEST_OK(src, "procedure p; Select p such that Calls(_, p)", "B", "C", "D", "E", "F", "G", "H", "X", "P");
}

TEST_CASE("Calls(_, _)")
{
    TEST_OK(src, "print p; Select p such that Calls(_, _)", 8, 14, 15, 16);
    TEST_EMPTY("procedure a { x = 1; }", "procedure p; Select p such that Calls(_, _)");
}
