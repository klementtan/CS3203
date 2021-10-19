// test_affects.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto prog_1 = R"(
    procedure A {
        while (a==5) {
            while (a==5) {
                while (a == 5) {
                    a=5;
                }
            }
        }
    }
    procedure B {
        if (x==1) then {
            x = x+1; }
        else {
            if (x==2) then {
                x = 222;
            } else {
                x = 333;
            }
        }
    }
)";

constexpr const auto prog_2 = R"(
    procedure First {
        while (a==5) {
            if (a==5) then {
                while (a==5) {
                    x = 3;
                }
            } else {
                if (a==5) then {
                    x = 1;
                } else {
                    x = 2;
                }
            }
        }
    }
    procedure Second {
        x = 0;
        i = 5;
        while (i!=0) {
            x = x + 2*y;
            call First;
            i = i - 1; }
        if (x==1) then {
            x = x+1; }
        else {
            if (x==2) then {
                x = 222;
            } else {
                x = 333;
            }
        }
        z = z + x + i;
        y = z + 2;
        x = x * y + z; }
)";


constexpr const auto prog_3 = R"(
    procedure First {
        x = 0;
        i = 5;
        while (i!=0) {
            x = x + 2*y;
            read x;
            y = 2;
            i = i - 1; }
        if (x==1) then {
            x = x+1; }
        else {
            if (x==2) then {
                i = 222;
            } else {
                x = 333;
            }
        }
        z = z + x + i;
        y = z + 2;
        x = x * y + z; }
)";

constexpr const auto prog_4 = R"(
    procedure Second {
        x = 0;
        i = 5;
        while (i!=0) {
            x = x + 2*y;
            a = 1;
            i = i - 1; }
        if (x==1) then {
            x = x+1; }
        else {
            z = 1; }
        z = z + x + i;
        y = z + 2;
        a = z * 3;
        b = a + 3;
        c = b + b; }
)";


TEST_CASE("Affects")
{
    SECTION("Straightforward")
    {
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(8, 11)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(9, 13)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(19, 20)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(19, 21)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(20, 21)", "TRUE");
    }

    SECTION("Prefer to skip while")
    {
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(8, 15)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(9, 19)", "TRUE");
    }

    SECTION("Prefer one branch")
    {
        TEST_OK(prog_3, "Select BOOLEAN such that Affects(1, 13)", "TRUE");
        TEST_OK(prog_3, "Select BOOLEAN such that Affects(2, 13)", "TRUE");
    }

    SECTION("Negative test case: modified by procedure call")
    {
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(11, 15)", "FALSE");
    }

    SECTION("Negative test case: modified by read statement")
    {
        TEST_OK(prog_3, "Select BOOLEAN such that Affects(4, 9)", "FALSE");
    }

    SECTION("Negative test case: cannot skip over both branches")
    {
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(8, 19)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(8, 21)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(11, 19)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(11, 21)", "FALSE");
    }

    SECTION("Negative test case: not assign statements")
    {
        TEST_OK(prog_1, "Select BOOLEAN such that Affects(1, 3)", "FALSE");
        TEST_OK(prog_1, "Select BOOLEAN such that Affects(1, 4)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(4, 5)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(14, 16)", "FALSE");
    }

    SECTION("Negative test case: Next*(a, b) doesn't hold")
    {
        TEST_OK(prog_1, "Select BOOLEAN such that Affects(5, 5)", "FALSE");
        TEST_OK(prog_1, "Select BOOLEAN such that Affects(6, 5)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(6, 7)", "FALSE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects(21, 8)", "FALSE");
    }
}

TEST_CASE("Affects*")
{
    SECTION("Affects(a,b) holds")
    {
        TEST_OK(prog_2, "Select BOOLEAN such that Affects*(8, 11)", "TRUE");
        TEST_OK(prog_2, "Select BOOLEAN such that Affects*(9, 13)", "TRUE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 4)", "TRUE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 10)", "TRUE");
    }

    SECTION("One intermediary variable")
    {
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 11)", "TRUE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 12)", "TRUE");
    }

    SECTION("Two or more intermediary variables")
    {
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 13)", "TRUE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(1, 14)", "TRUE");
    }

    SECTION("Negative test case: not assignment statements")
    {
        TEST_OK(prog_3, "Select BOOLEAN such that Affects*(1, 3)", "FALSE");
        TEST_OK(prog_3, "Select BOOLEAN such that Affects*(3, 5)", "FALSE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(6, 7)", "FALSE");
    }

    SECTION("Negative test case: Next*(a,b) doesn't hold")
    {
        TEST_OK(prog_3, "Select BOOLEAN such that Affects*(3, 1)", "FALSE");
        TEST_OK(prog_4, "Select BOOLEAN such that Affects*(8, 9)", "FALSE");
    }
}
