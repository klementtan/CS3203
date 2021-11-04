// test_next.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

// clang-format off
constexpr const auto src1 = R"(
    procedure A {
        a = 0;
        b = 0;
        c = 0;
    }
    procedure B {
        d = 0;
        if (1 == 1) then {
            e = 0;
            f = 0;
        } else {
            g = 0;
            h = 0;
        }
        x = 0;
    }
)";

constexpr const auto src2 = R"(
    procedure A {
        d = 0;
        while (69 < 420) {
            print x;
            print y;
        }
        read z;
    }
)";
// clang-format on

TEST_CASE("Next(stmt, stmt)")
{
    TEST_OK(src1, "Select BOOLEAN such that Next(1, 2)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(2, 3)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(4, 5)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(5, 6)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(6, 7)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(8, 9)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(9, 10)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(7, 10)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next(1, 3)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next(3, 2)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next(3, 1)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next(7, 8)", "FALSE");

    TEST_OK(src1, "Select BOOLEAN such that Next(5, 10)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next(3, 4)", "FALSE");

    TEST_OK(src2, "Select BOOLEAN such that Next(1, 2)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next(2, 3)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next(3, 4)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next(4, 2)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next(2, 5)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next(4, 5)", "FALSE");
}

TEST_CASE("Next(stmt, decl)")
{
    TEST_OK(src1, "stmt s; Select s such that Next(1, s)", 2);
    TEST_OK(src1, "stmt s; Select s such that Next(3, s)");

    TEST_OK(src1, "stmt s; Select s such that Next(s, 2)", 1);
    TEST_OK(src1, "stmt s; Select s such that Next(s, 3)", 2);
}

TEST_CASE("Next(decl, decl)")
{
    TEST_OK(src1, "assign a, b; Select <a, b> such that Next(a, b)", "1 2", "2 3", "6 7", "8 9", "7 10", "9 10");
}

TEST_CASE("Next(_, _)")
{
    TEST_OK(src1, "Select BOOLEAN such that Next(_, _)", "TRUE");
    TEST_OK("procedure A { print x; }", "Select BOOLEAN such that Next(_, _)", "FALSE");
}



TEST_CASE("Next*(stmt, stmt)")
{
    TEST_OK(src1, "Select BOOLEAN such that Next*(1, 2)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(2, 3)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(4, 5)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(5, 6)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(6, 7)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(8, 9)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(9, 10)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(7, 10)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(1, 3)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(5, 10)", "TRUE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(3, 2)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(3, 1)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(7, 8)", "FALSE");
    TEST_OK(src1, "Select BOOLEAN such that Next*(1, 4)", "FALSE");

    TEST_OK(src2, "Select BOOLEAN such that Next*(1, 2)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(2, 3)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(3, 4)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(4, 2)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(2, 5)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(2, 2)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(3, 3)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(4, 4)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(4, 5)", "TRUE");
    TEST_OK(src2, "Select BOOLEAN such that Next*(5, 4)", "FALSE");
}

TEST_CASE("Next*(stmt, decl)")
{
    TEST_OK(src1, "stmt s; Select s such that Next*(1, s)", 2, 3);
    TEST_OK(src1, "stmt s; Select s such that Next*(3, s)");
    TEST_OK(src1, "stmt s; Select s such that Next*(s, 2)", 1);
    TEST_OK(src1, "stmt s; Select s such that Next*(s, 3)", 1, 2);

    TEST_OK(src2, "stmt s; Select s such that Next*(1, s)", 2, 3, 4, 5);
    TEST_OK(src2, "stmt s; Select s such that Next*(2, s)", 2, 3, 4, 5);
    TEST_OK(src2, "stmt s; Select s such that Next*(3, s)", 2, 3, 4, 5);
    TEST_OK(src2, "stmt s; Select s such that Next*(4, s)", 2, 3, 4, 5);
    TEST_OK(src2, "stmt s; Select s such that Next*(5, s)");

    TEST_OK(src2, "stmt s; Select s such that Next*(s, 1)");
    TEST_OK(src2, "stmt s; Select s such that Next*(s, 2)", 1, 2, 3, 4);
    TEST_OK(src2, "stmt s; Select s such that Next*(s, 3)", 1, 2, 3, 4);
    TEST_OK(src2, "stmt s; Select s such that Next*(s, 4)", 1, 2, 3, 4);
    TEST_OK(src2, "stmt s; Select s such that Next*(s, 5)", 1, 2, 3, 4);
}

TEST_CASE("Next*(decl, decl)")
{
    TEST_OK(src1, "assign a, b; Select <a, b> such that Next*(a, b)", "1 2", "2 3", "1 3", "4 6", "4 7", "4 8", "4 9",
        "4 10", "6 7", "6 10", "8 9", "8 10", "7 10", "9 10");
}

TEST_CASE("Next*(_, _)")
{
    TEST_OK(src1, "Select BOOLEAN such that Next*(_, _)", "TRUE");
    TEST_OK("procedure A { print x; }", "Select BOOLEAN such that Next*(_, _)", "FALSE");
}


constexpr const auto prog1 = R"(
procedure Bill {
      x = 5;
      call Mary;
      y = x + 6;
      call John;
      z = x * y + 2; }

procedure Mary {
      y = x * 3;
      call John;
      z = x + y; }

procedure John {
      if (i > 0) then {
              x = x + z; }
      else {
              y = x * y; } }
)";

constexpr const auto prog2 = R"(
procedure B {
        call C;
        call C;
        call C; }

procedure C {
        d = a;
        a = b;
        b = c;
        c = d; }
)";

constexpr const auto prog3 = R"(
procedure P {
      call Q1;
      while (i > 0) {
              call Q1; }
      if (i > 0) then {
              call Q2; }
        else {
              call Q3; } }

procedure Q1 {
      call Q2;}

procedure Q2 {
      call Q3;}

procedure Q3 {
      a = a;}
)";



TEST_CASE("NextBip(stmt, stmt)")
{
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(2, 6)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(9, 10)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(9, 11)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(11, 8)", "TRUE");

    TEST_OK(prog2, "Select BOOLEAN such that NextBip(1, 4)", "TRUE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip(7, 3)", "TRUE");

    TEST_OK(prog3, "Select BOOLEAN such that NextBip(9, 2)", "TRUE");

    TEST_OK(prog1, "Select BOOLEAN such that NextBip(2, 7)", "FALSE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(2, 3)", "FALSE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(10, 11)", "FALSE");

    TEST_OK(prog2, "Select BOOLEAN such that NextBip(1, 7)", "FALSE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip(1, 2)", "FALSE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip(7, 7)", "FALSE");

    TEST_OK(prog3, "Select BOOLEAN such that NextBip(9, 6)", "FALSE");
    TEST_OK(prog3, "Select BOOLEAN such that NextBip(3, 2)", "FALSE");
}

TEST_CASE("NextBip(stmt, _)")
{
    TEST_OK(prog1, "Select BOOLEAN such that NextBip(1, _)", "TRUE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip(1, _)", "TRUE");
    TEST_OK(prog3, "Select BOOLEAN such that NextBip(1, _)", "TRUE");
}

TEST_CASE("NextBip(stmt, decl)")
{
    TEST_OK(prog1, "prog_line s; Select s such that NextBip(2, s)", 6);
    TEST_OK(prog1, "prog_line s; Select s such that NextBip(8, s)", 3);
    TEST_OK(prog1, "prog_line s; Select s such that NextBip(11, s)", 5, 8);
    TEST_OK(prog1, "prog_line s; Select s such that NextBip(9, s)", 10, 11);

    TEST_OK(prog2, "prog_line s; Select s such that NextBip(1, s)", 4);
    TEST_OK(prog2, "prog_line s; Select s such that NextBip(7, s)", 2, 3);

    TEST_OK(prog3, "prog_line s; Select s such that NextBip(9, s)", 2);

    TEST_EMPTY(prog1, "prog_line s; Select s such that NextBip(5, s)");
}


TEST_CASE("NextBip*(stmt, stmt)")
{
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(2, 3)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(2, 6)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(1, 11)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(1, 5)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(4, 5)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(9, 5)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(10, 11)", "TRUE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(11, 11)", "TRUE");

    TEST_OK(prog2, "Select BOOLEAN such that NextBip*(3, 4)", "TRUE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip*(1, 3)", "TRUE");

    TEST_OK(prog3, "Select BOOLEAN such that NextBip*(9, 4)", "TRUE");

    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(4, 8)", "FALSE");
    TEST_OK(prog1, "Select BOOLEAN such that NextBip*(6, 2)", "FALSE");
    TEST_OK(prog2, "Select BOOLEAN such that NextBip*(3, 1)", "FALSE");
}


TEST_CASE("NextBip*(stmt, decl)")
{
    TEST_OK(prog1, "prog_line s; Select s such that NextBip*(2, s)", 3, 4, 5, 6, 7, 8, 9, 10, 11);
    TEST_OK(prog1, "prog_line s; Select s such that NextBip*(11, s)", 3, 4, 5, 8, 9, 10, 11);
}
