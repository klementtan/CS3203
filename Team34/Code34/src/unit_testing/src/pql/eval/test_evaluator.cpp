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

constexpr const auto prog_2 = R"(
procedure A {
    a0 = 0; a1 = 1; a2 = 2; a3 = 3; a4 = 4; a5 = 5; a6 = 6; a7 = 7;
    a8 = 8; a9 = 9; aA = A; aB = B; aC = C; aC = C; aE = E; aF = F;
}
)";



TEST_CASE("No such that")
{
    TEST_OK(prog_1, "stmt a; Select a", 1, 2, 3);
    TEST_OK(prog_1, "prog_line a; Select a", 1, 2, 3);
}

TEST_CASE("boolean edge cases")
{
    TEST_OK(prog_1, "stmt a, b; Select BOOLEAN such that Follows(a, b) with a.stmt# = b.stmt#", "FALSE");
    TEST_OK(prog_1, "stmt a, b; Select BOOLEAN with a.stmt# = b.stmt# such that Follows(a, b)", "FALSE");

    TEST_OK(prog_2,
        "assign a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,aA,aB,aC,aD,aE,aF; Select BOOLEAN such that "
        "Follows(a0,a1) and Follows(a1,a2) and Follows(a2,a3) and Follows(a3,a4) and Follows(a4,a5) and "
        "Follows(a5,a6) and Follows(a6,a7) and Follows(a7,a8) and Follows(a8,a9) and Follows(a9,aA) and "
        "Follows(aA,aB) and Follows(aB,aC) and Follows(aC,aD) and Follows(aD,aE) and Follows(aE,aF)",
        "TRUE");

    TEST_OK(prog_2,
        "assign a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,aA; Select BOOLEAN such that Follows(a0,a1)"
        " and Follows(a1,a2) and Follows(a2,a3) and Follows(a3,a4) and Follows(a4,a5) and Follows(a5,a6)"
        " and Follows(a6,a7) and Follows(a7,a8) and Follows(a8,a9) and Follows(a9,aA)",
        "TRUE");

    TEST_OK(prog_2,
        "assign a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,aA,aB,aC,aD,aE,aF,aG; Select BOOLEAN such that "
        "Follows(a0,a1) and Follows(a1,a2) and Follows(a2,a3) and Follows(a3,a4) and Follows(a4,a5) and "
        "Follows(a5,a6) and Follows(a6,a7) and Follows(a7,a8) and Follows(a8,a9) and Follows(a9,aA) and "
        "Follows(aA,aB) and Follows(aB,aC) and Follows(aC,aD) and Follows(aD,aE) and Follows(aE,aF) and "
        "Follows(aF,aG)",
        "FALSE");
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


TEST_CASE("prog_line")
{
    // check that prog_line and stmt are interchangeable
    TEST_OK(prog_1, "prog_line a, b; Select <a, b> such that Follows(a, b)", "1 2", "2 3");
}

TEST_CASE("bad arguments")
{
    TEST_EMPTY(prog_1, "procedure a, b; Select <a, b> such that Follows(a, b)");
}