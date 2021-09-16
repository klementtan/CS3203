// test_ast.cpp
//
// Unit test for pql/eval/evaluator.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include <iostream>
#include "runner.h"

#include "pql/parser/parser.h"
#include "pql/eval/evaluator.h"
#include "pkb.h"
#include "simple/parser.h"
#include <list>

constexpr const auto prog_1 = R"(
procedure A {
    a = 1;
    b = 2;
    c = 3;
}
)";
TEST_CASE("No such that")
{
    TEST_OK(prog_1, "stmt a; Select a", 1, 2, 3);
}

TEST_CASE("Check valid domain")
{
    SECTION("Involved query has empty domain")
    {
        TEST_EMPTY(prog_1, "stmt a1, a2; variable v; Select a2 such that Uses(a1,v)");
    }

    SECTION("Ignore not involved declaration")
    {
        TEST_OK(prog_1, "stmt a; if ifs; Select a", 1, 2, 3);
    }
}

TEST_CASE("Constant")
{
    SECTION("Should return all constants")
    {
        TEST_OK(prog_1, "constant c; Select c", 1, 2, 3);
    }
}

TEST_CASE("Procedure")
{
    SECTION("Should return all constants")
    {
        TEST_OK(prog_1, "procedure p; Select p", "A");
    }
}

TEST_CASE("Variable")
{
    SECTION("Should return all variables")
    {
        TEST_OK(prog_1, "variable v; Select v", "a", "b", "c");
    }
}
