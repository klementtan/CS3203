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
    TEST_OK(prog_1, "prog_line a; Select a", 1, 2, 3);
}

TEST_CASE("boolean edge case")
{
    TEST_OK(prog_1, "stmt a, b; Select BOOLEAN such that Follows(a, b) with a.stmt# = b.stmt#", "FALSE");
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




TEST_CASE("temp")
{
    auto prog = R"(
    procedure procedure {
  else = 2;
  z = 3;
  read = 5;
  while (i!=0) {
    x = x - 1;
    if (x==1) then {
      z = x + 1; }
    else {
      y = then + x; }
    if = z + x + i;
    call q;
    i = i - while; }
  call p; }

procedure p {
  if (x<0) then {
    while (i>0) {
      x = z * 3 + 2 * y;
      call q;
      call = i - 1; }
    x = x + 1;
    z = x + z; }
  else {
    z = 1; }
  z = z + x + i; }

procedure q {
  if (x==1) then {
    z = if + 1; }
  else {
    while = z + x; } }

procedure if {
    else = if + while + call + procedure;
}
    )";

    TEST_OK(prog,
        "stmt s1, s2; procedure p; variable v; "
        "Select BOOLEAN such that Follows*(s1, s2) with s1.stmt# = s2.stmt#",
        "FALSE");
}