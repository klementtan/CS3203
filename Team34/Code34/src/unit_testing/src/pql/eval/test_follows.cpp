// test_follows.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto prog_1 = R"(
procedure A {
    a = 1;
    b = 1;
    c = 1;
}
)";

constexpr const auto prog_2 = R"(
procedure A {
    a = 1;
}
procedure B {
    b = 1;
}
)";


TEST_CASE("Follows(DeclaredStmt, _)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows(a, _)", 1, 2);
}

TEST_CASE("Follows(DeclaredStmt, StmtId)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows(a, 2)", 1);
}

TEST_CASE("Follows(DeclaredStmt, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a, b; Select b such that Follows(a, b)", 2, 3);

    TEST_EMPTY(prog_2, "stmt a, b; Select a such that Follows(a, b)");
    TEST_EMPTY(prog_2, "stmt a, b; Select b such that Follows(a, b)");
}

TEST_CASE("Follows(StmtId, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows(1, a)", 2);
}

TEST_CASE("Follows(_, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows(_, a)", 2, 3);
}

TEST_CASE("Follows(_, _)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows(_, _)", 1, 2, 3);
    TEST_OK("procedure x{a=1;b=1;c=1;} procedure y{d=1;}", "stmt a; Select a such that Follows(_, _)", 1, 2, 3, 4);
}


TEST_CASE("Follows*(DeclaredStmt, _)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows*(a, _)", 1, 2);
}

TEST_CASE("Follows*(_, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows*(_, a)", 2, 3);
}

TEST_CASE("Follows*(_, _)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows*(_, _)", 1, 2, 3);
}

TEST_CASE("Follows*(StmtId, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows*(1, a)", 2, 3);
}

TEST_CASE("Follows*(DeclaredStmt, StmtId)")
{
    TEST_OK(prog_1, "stmt a; Select a such that Follows*(a, 3)", 1, 2);
}

TEST_CASE("Follows*(DeclaredStmt, DeclaredStmt)")
{
    TEST_OK(prog_1, "stmt a, b; Select b such that Follows*(a, b)", 2, 3);

    TEST_EMPTY(prog_2, "stmt c, d; Select c such that Follows*(c, d)");
    TEST_EMPTY(prog_2, "stmt c, d; Select d such that Follows*(c, d)");
}

TEST_CASE("no follows")
{
    TEST_EMPTY("procedure a { x = 1; }", R"(variable v; Select v such that Follows(_, _))");
}
