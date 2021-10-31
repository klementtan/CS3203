// test_with.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

constexpr const auto prog_1 = R"(
      procedure First {
      read x;
      read z;
      call Second; }

      procedure Second {
          x = 0;
          i = 5;
          while (i!=0) {
              x = x + 2*y;
              call Third;
              i = i - 1; }
          if (x==1) then {
              x = x+1; }
          else {
              z = 1; }
          z = z + x + i;
          y = z + 2;
          x = x * y + z; }

      procedure Third {
          z = 5;
          v = z;
          print v; }
)";

constexpr const auto prog_2 = R"(
procedure a
{
    print c;
    read b;
}
procedure b
{
    print a;
    read c;
    call a;
}
procedure c
{
    print b;
    read a;
    call a;
}
)";

constexpr const auto prog_3 = R"(
procedure a
{
    read b;
}

procedure b
{
    read x;
}
)";

TEST_CASE("with decl/decl")
{
    TEST_OK(prog_1, "prog_line a, b; Select <a,b> with a = b", "1 1", "2 2", "3 3", "4 4", "5 5", "6 6", "7 7", "8 8",
        "9 9", "10 10", "11 11", "12 12", "13 13", "14 14", "15 15", "16 16", "17 17", "18 18");
}

TEST_CASE("with decl/number")
{
    TEST_OK(prog_1, "prog_line a; Select a with a = 3", "3");
    TEST_EMPTY(prog_1, "prog_line a; Select a with a = 69");

    TEST_EMPTY(prog_1, "stmt a; Select a with a = 69");
    TEST_EMPTY(prog_1, "call a; Select a with a = 69");


    TEST_OK(prog_1, "prog_line a; Select a with 3 = a", "3");
    TEST_EMPTY(prog_1, "prog_line a; Select a with 69 = a");

    TEST_EMPTY(prog_1, "stmt a; Select a with 69 = a");
    TEST_EMPTY(prog_1, "call a; Select a with 69 = a");
}

TEST_CASE("with decl/string")
{
    TEST_EMPTY(prog_1, "prog_line a; Select a with a = \"3\"");
    TEST_EMPTY(prog_1, "stmt a; Select a with a = \"foo\"");
    TEST_EMPTY(prog_1, "call a; Select a with a = \"Third\"");

    TEST_EMPTY(prog_1, "prog_line a; Select a with \"3\" = a");
    TEST_EMPTY(prog_1, "stmt a; Select a with \"foo\" = a");
    TEST_EMPTY(prog_1, "call a; Select a with \"Third\" = a");
}

TEST_CASE("with decl/stmt#")
{
    TEST_OK(prog_1, "prog_line a; stmt s; Select a with a = s.stmt#", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18);

    TEST_OK(prog_1, "prog_line a; call s; Select a with a = s.stmt#", 3, 8);
    TEST_OK(prog_1, "prog_line a; print s; Select a with a = s.stmt#", 18);
    TEST_OK(prog_1, "prog_line a; if s; Select a with a = s.stmt#", 10);

    TEST_OK(prog_2, "prog_line a; print s; Select a such that Uses(s, \"a\") with s.stmt# = a", 3);
}

TEST_CASE("with stmt#/stmt#")
{
    TEST_EMPTY(prog_1, "prog_line a; Select a with a.stmt# = 69");
    TEST_EMPTY(prog_1, "stmt a; Select a with a.stmt# = 69");
    TEST_EMPTY(prog_1, "call a; print b; Select <a, b> with a.stmt# = b.stmt#");

    TEST_OK(prog_1, "stmt a, b; Select <a, b> with a.stmt# = b.stmt#", "1 1", "2 2", "3 3", "4 4", "5 5", "6 6", "7 7",
        "8 8", "9 9", "10 10", "11 11", "12 12", "13 13", "14 14", "15 15", "16 16", "17 17", "18 18");

    TEST_OK(prog_1, "call a; stmt b; Select <a, b> with a.stmt# = b.stmt#", "3 3", "8 8");
}

TEST_CASE("with stmt#/value")
{
    TEST_OK(prog_1, "stmt a; constant c; Select a with a.stmt# = c.value", 2, 5, 1);
    TEST_OK(prog_1, "constant a, c; Select <a, c> with a.value = c.value", "0 0", "2 2", "5 5", "1 1");

    TEST_EMPTY(prog_1, "prog_line x; constant a; Select a with x.stmt# = a.value");
}

TEST_CASE("with stmt#/number")
{
    TEST_EMPTY(prog_1, "prog_line a; Select a with a.stmt# = 69");
    TEST_EMPTY(prog_1, "stmt a; Select a with a.stmt# = 69");

    TEST_EMPTY(prog_1, "prog_line a; Select a with 69 = a.stmt#");
    TEST_EMPTY(prog_1, "stmt a; Select a with 69 = a.stmt#");

    TEST_OK(prog_1, "stmt a; Select a with a.stmt# = 1", 1);
    TEST_OK(prog_1, "stmt a; Select a with a.stmt# = 10", 10);

    TEST_OK(prog_1, "stmt a; Select a with 1 = a.stmt#", 1);
    TEST_OK(prog_1, "stmt a; Select a with 10 = a.stmt#", 10);
}

TEST_CASE("with procName")
{
    TEST_OK(prog_1, "procedure p; Select p with p.procName = \"Third\"", "Third");
    TEST_OK(prog_1, "procedure p; Select p with \"Third\" = p.procName", "Third");

    TEST_OK(prog_1, "call c; procedure p; Select <c, p> with c.procName = p.procName", "3 Second", "8 Third");
    TEST_OK(prog_1, "call c; procedure p; Select <c, p> with p.procName = c.procName", "3 Second", "8 Third");
}

TEST_CASE("with varName")
{
    TEST_OK(prog_2, "variable v; Select v with v.varName = \"a\"", "a");
    TEST_OK(prog_2, "variable v; Select v with \"a\" = v.varName", "a");

    TEST_OK(prog_2, "read c; variable v; Select <c, v> with c.varName = v.varName", "2 b", "4 c", "7 a");
    TEST_OK(prog_2, "read c; variable v; Select <c, v> with v.varName = c.varName", "2 b", "4 c", "7 a");

    TEST_OK(prog_2, "print c; variable v; Select <c, v> with c.varName = v.varName", "1 c", "3 a", "6 b");
    TEST_OK(prog_2, "print c; variable v; Select <c, v> with v.varName = c.varName", "1 c", "3 a", "6 b");

    TEST_OK(prog_2, "read r; procedure p; Select r.varName with r.varName = p.procName", "a", "b", "c");
    TEST_OK(prog_3, "read r; procedure p; Select r.varName with p.procName = r.varName", "b");
}
