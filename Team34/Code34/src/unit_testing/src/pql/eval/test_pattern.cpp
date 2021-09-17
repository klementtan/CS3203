// test_pattern.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pkb.h"
#include "simple/parser.h"
#include "pql/parser/parser.h"
#include "pql/eval/evaluator.h"

constexpr const auto prog_1 = R"(
procedure Example {
    x = 6;
    y = 18;
    z = 12;

    t1 = x + y + z;
    t2 = x + y + z - z * z;
    t3 = y * (w + x) - (w - y * z) - (w - x) - z;

    t4 = a + b * c - d / e + f * g - h % (i + j - k) * l / ((m - m) * (n + o));

    x = 6 + 1 / 2 - 3;
    y = 69 * 420 - 123;
    z = a + (b + c);

    a1 = 5 * (a + 7 + x);
    a2 = 5 * (a + 7 + x);
    a3 = 5 * (a + 7 + x);
    read q;
}
)";

constexpr const auto prog_2 = R"(
procedure foo {
    print x;
}
procedure bar {
    print y;
}
)";

constexpr const auto prog_3 = R"(
    procedure main {
      flag = 0;
      call computeCentroid;
      call printResults;
    }
    procedure readPoint {
        read x;
        read y;
    }
    procedure printResults {
        print flag;
        print cenX;
        print cenY;
        print normSq;
    }
    procedure computeCentroid {
        count = 0;
        cenX = 0;
        cenY = 0;
        call readPoint;
        while ((x != 0) && (y != 0)) {
            count = count + 1;
            cenX = cenX + x;
            cenY = cenY + y;
            call readPoint;
        }
        if (count == 0) then {
            flag = 1;
        } else {
            cenX = cenX / count;
            cenY = cenY / count;
        }
        normSq = cenX * cenX + cenY * cenY;
    }
)";

TEST_CASE("Select pattern assign(name, _)")
{
    SECTION("positive cases")
    {
        TEST_OK(prog_1, R"(assign a; Select a pattern a("x", _))", 1, 8);
        TEST_OK(prog_1, R"(assign a; Select a pattern a("y", _))", 2, 9);
        TEST_OK(prog_1, R"(assign a; Select a pattern a("z", _))", 3, 10);
    }

    SECTION("empty cases")
    {
        TEST_EMPTY(prog_1, "assign a;Select a pattern a(\"q\", _)");
    }
}

TEST_CASE("Select pattern assign(name, _subexpr_)")
{
    // we're running a lot of things, so save time here by only processing once
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    SECTION("xyz")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("x", _"1"_))^", 8);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("x", _"6"_))^", 1, 8);

        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("y", _"69"_))^", 9);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("z", _"12"_))^", 3);
    }

    SECTION("t1")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", _"(x)"_))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", _"(y)"_))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", _"(z)"_))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", _"x + y"_))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", _"x + y + z"_))^", 4);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t1", _"y + z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t1", _"69"_))^");
    }

    SECTION("t2")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"(x)"_))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"(y)"_))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"(z)"_))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"x + y"_))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"z * z"_))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", _"x + y + z"_))^", 5);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", _"y + z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", _"z - z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", _"420"_))^");
    }

    SECTION("t3")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"(x)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"(y)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"(z)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"w + x"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"y * z"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"y * (w + x)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"w - (y * z)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", _"y * (w + x) - (w - y * z)"_))^", 6);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", _"w - y"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", _"y * w"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", _"(w - x) - z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", _"asdf"_))^");
    }

    SECTION("t4")
    {
        // for reference:
        // ((((a + (b * c)) - (d / e)) + (f * g)) - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"(a)"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"(f)"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"(o)"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"f * g"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"d / e"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"b * c"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"i + j"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"m - m"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"i + j - k"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"a + b * c"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", _"h % (i + j - k)"_))^", 7);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"a + b"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"j - k"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"h % i"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"c - d"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"m * n"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", _"g - h % i"_))^");
    }
}

TEST_CASE("Select pattern assign(name, fullexpr)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    SECTION("xyz")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("x", "6"))^", 1);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("y", "18"))^", 2);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("z", "12"))^", 3);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("x", "3"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("x", "2"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("y", "69"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("z", "a"))^");
    }

    SECTION("t1")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", "x + y + z"))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t1", "(x + y) + z"))^", 4);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t1", "x + y"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t1", "y + z"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t1", "x + (y + z)"))^");
    }

    SECTION("t2")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", "x + y + z - z * z"))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t2", "((x + y) + z) - (z * z)"))^", 5);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", "x + y"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", "z * z"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t2", "x + y + z"))^");
    }

    SECTION("t3")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", "y * (w + x) - (w - y * z) - (w - x) - z"))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t3", "((y * (w + x)) - (w - (y * z))) - (w - x) - z"))^", 6);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", "w + x"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", "y * w"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", "w - y * z"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t3", "w - x"))^");
    }

    SECTION("t4")
    {
        // for reference:
        // ((((a + (b * c)) - (d / e)) + (f * g)) - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))
        TEST_OK(pkb.get(),
            R"^(assign a; Select a pattern a("t4", "a + b * c - d / e + f * g - h % (i + j - k) * l / ((m - m) * (n + o))"))^",
            7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a("t4", "((((a + (b * c)) - (d / e)) + (f * g))
            - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))"))^",
            7);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "d / e"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "i + j"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "h % i"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "c - d"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "m * n"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "b * c"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a("t4", "g - h % i"))^");
    }
}

TEST_CASE("Select pattern assign(decl, _)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    TEST_OK(pkb.get(), R"^(assign a; variable v; Select a pattern a(v, _))^", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _))^", "a1", "a2", "a3", "t1", "t2", "t3", "t4", "x",
        "y", "z");

    TEST_EMPTY(prog_2, "assign a; variable v; Select v pattern a(v, _)");
    TEST_EMPTY(prog_2, "assign a; variable v; Select a pattern a(v, _)");
}

TEST_CASE("Select pattern assign(decl, _subexpr_)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"6"_))^", "x");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"b + c"_))^", "z");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"x + y"_))^", "t1", "t2");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"a + 7"_))^", "a1", "a2", "a3");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"a + 7 + x"_))^", "a1", "a2", "a3");

    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"y + z"_))^");
    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"c - d"_))^");
    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"5 * a"_))^");
    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, _"7 + x"_))^");
}

TEST_CASE("Select pattern assign(decl, fullexpr)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "6"))^", "x");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "18"))^", "y");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "12"))^", "z");

    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "x + y + z"))^", "t1");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "x + y + z - z * z"))^", "t2");
    TEST_OK(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "y * (w + x) - (w - y * z) - (w - x) - z"))^", "t3");

    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "a + 7"))^");
    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "1 / 2"))^");
    TEST_EMPTY(pkb.get(), R"^(assign a; variable v; Select v pattern a(v, "g - h"))^");
}

TEST_CASE("Select pattern assign(_, _)")
{
    TEST_OK(prog_1, R"^(assign a; Select a pattern a(_, _))^", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
}


TEST_CASE("Select pattern assign(_, _subexpr_)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    SECTION("a")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"1"_))^", 8);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"6"_))^", 1, 8);

        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"69"_))^", 9);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"12"_))^", 3);
    }

    SECTION("b")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"(x)"_))^", 4, 5, 6, 11, 12, 13);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"(y)"_))^", 4, 5, 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"(w)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"x + y"_))^", 4, 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"x + y + z"_))^", 4, 5);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"y + z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"70"_))^");
    }

    SECTION("c")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"w + x"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"y * z"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"y * (w + x)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"w - (y * z)"_))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"y * (w + x) - (w - y * z)"_))^", 6);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"w - y"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"y * w"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"(w - x) - z"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"asdf"_))^");
    }

    SECTION("d")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"(f)"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"(o)"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"f * g"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"d / e"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"b * c"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"i + j"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"m - m"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"i + j - k"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"a + b * c"_))^", 7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, _"h % (i + j - k)"_))^", 7);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"a + b"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"j - k"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"h % i"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"c - d"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"m * n"_))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, _"g - h % i"_))^");
    }
}

TEST_CASE("Select pattern assign(_, fullexpr)")
{
    auto prog = simple::parser::parseProgram(prog_1);
    auto pkb = pkb::processProgram(std::move(prog));

    SECTION("xyz")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "6"))^", 1);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "18"))^", 2);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "12"))^", 3);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "3"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "2"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "69"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "a"))^");
    }

    SECTION("t1")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "x + y + z"))^", 4);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "(x + y) + z"))^", 4);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "x + y"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "y + z"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "x + (y + z)"))^");
    }

    SECTION("t2")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "x + y + z - z * z"))^", 5);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "((x + y) + z) - (z * z)"))^", 5);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "x + y"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "z * z"))^");
    }

    SECTION("t3")
    {
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "y * (w + x) - (w - y * z) - (w - x) - z"))^", 6);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "((y * (w + x)) - (w - (y * z))) - (w - x) - z"))^", 6);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "w + x"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "y * w"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "w - y * z"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "w - x"))^");
    }

    SECTION("t4")
    {
        TEST_OK(pkb.get(),
            R"^(assign a; Select a pattern a(_, "a + b * c - d / e + f * g - h % (i + j - k) * l / ((m - m) * (n + o))"))^",
            7);
        TEST_OK(pkb.get(), R"^(assign a; Select a pattern a(_, "((((a + (b * c)) - (d / e)) + (f * g))
            - (((h % ((i + j) - k)) * l) / ((m - m) * (n + o))))"))^",
            7);

        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "d / e"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "i + j"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "h % i"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "c - d"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "m * n"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "b * c"))^");
        TEST_EMPTY(pkb.get(), R"^(assign a; Select a pattern a(_, "g - h % i"))^");
    }

    SECTION("t5")
    {
        TEST_OK(prog_3, R"^(assign a; while w; Select a such that Parent* (w, a) pattern a ("count", _))^", 15);
        TEST_OK(prog_3, R"^(assign a; variable v; Select a such that Uses (a, v) pattern a (v, _"x"_))^", 16);
    }
}
