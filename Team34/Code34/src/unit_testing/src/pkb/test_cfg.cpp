// cfg.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "design_extractor.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

#include <zpr.h>
using namespace simple::parser;
using namespace pkb;

constexpr const auto sample_source_A = R"(
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


constexpr const auto sample_source_B = R"(
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


constexpr const auto sample_source_C = R"(
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

constexpr const auto sample_source_D = R"(
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

static auto kb1 = DesignExtractor(parseProgram(sample_source_A)).run();
static auto cfg1 = kb1 -> getCFG();
static auto kb2 = DesignExtractor(parseProgram(sample_source_B)).run();
static auto cfg2 = kb2 -> getCFG();
static auto kb3 = DesignExtractor(parseProgram(sample_source_C)).run();
static auto cfg3 = kb3 -> getCFG();
static auto kb4 = DesignExtractor(parseProgram(sample_source_D)).run();
static auto cfg4 = kb4 -> getCFG();

TEST_CASE("CFG")
{
    const std::string mat_A = R"(
          001 002 003 004 005 006 007 008 009 010 011 012 013 014 015 016 017 018 019 020 021
    001 | 003 001 002 003 002 003 003 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    002 | 002 003 001 002 001 002 002 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    003 | 001 002 002 001 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    004 | 002 003 001 002 004 005 005 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    005 | 002 003 004 005 004 001 001 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    006 | 001 002 003 004 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    007 | 001 002 003 004 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    008 | 000 000 000 000 000 000 000 000 001 002 003 004 005 003 004 004 005 005 005 006 007
    009 | 000 000 000 000 000 000 000 000 000 001 002 003 004 002 003 003 004 004 004 005 006
    010 | 000 000 000 000 000 000 000 000 000 004 001 002 003 001 002 002 003 003 003 004 005
    011 | 000 000 000 000 000 000 000 000 000 003 004 001 002 004 005 005 006 006 006 007 008
    012 | 000 000 000 000 000 000 000 000 000 002 003 004 001 003 004 004 005 005 005 006 007
    013 | 000 000 000 000 000 000 000 000 000 001 002 003 004 002 003 003 004 004 004 005 006
    014 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 001 002 002 002 003 004
    015 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    016 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 001 002 003 004
    017 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    018 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    019 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002
    020 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001
    021 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    )";
    CHECK(mat_A.compare(cfg1->getMatRep()));

    const std::string mat_B = R"(
          001 002 003 004 005 006 007 008 009 010 011 012 013 014 015 016 017 018 019 020 021
    001 | 003 001 002 003 002 003 003 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    002 | 002 003 001 002 001 002 002 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    003 | 001 002 002 001 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    004 | 002 003 001 002 004 005 005 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    005 | 002 003 004 005 004 001 001 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    006 | 001 002 003 004 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    007 | 001 002 003 004 003 004 004 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    008 | 000 000 000 000 000 000 000 000 001 002 003 004 005 003 004 004 005 005 005 006 007
    009 | 000 000 000 000 000 000 000 000 000 001 002 003 004 002 003 003 004 004 004 005 006
    010 | 000 000 000 000 000 000 000 000 000 004 001 002 003 001 002 002 003 003 003 004 005
    011 | 000 000 000 000 000 000 000 000 000 003 004 001 002 004 005 005 006 006 006 007 008
    012 | 000 000 000 000 000 000 000 000 000 002 003 004 001 003 004 004 005 005 005 006 007
    013 | 000 000 000 000 000 000 000 000 000 001 002 003 004 002 003 003 004 004 004 005 006
    014 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 001 002 002 002 003 004
    015 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    016 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 001 002 003 004
    017 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    018 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002 003
    019 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001 002
    020 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 001
    021 | 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000 000
    )";
    CHECK(mat_B.compare(cfg2->getMatRep()));
}

TEST_CASE("Next(a,b)")
{
    SECTION("stmt->stmt")
    {
        CHECK(cfg1->isStatementNext(1, 2));
        CHECK(cfg1->isStatementNext(2, 3));
        CHECK(cfg2->getNextStatements(19).size() == 1);

        CHECK(cfg1->getPreviousStatements(2) == StatementSet { 1, 3 });
        CHECK(cfg2->isStatementNext(8, 9));
        CHECK(cfg2->getPreviousStatements(9) == StatementSet { 8 });
    }
    SECTION("while->while")
    {
        CHECK(cfg1->isStatementNext(1, 2));
        CHECK(cfg1->isStatementNext(2, 3));
        CHECK(cfg1->getNextStatements(2).size() == 2);
    }
    SECTION("while->stmt")
    {
        CHECK(cfg1->isStatementNext(3, 4));
        CHECK(cfg2->isStatementNext(3, 4));
        CHECK(cfg2->getNextStatements(1).size() == 1);
    }
    SECTION("while->if")
    {
        CHECK(cfg2->isStatementNext(1, 2));
    }
    SECTION("while beside if")
    {
        CHECK(cfg2->isStatementNext(10, 14));
    }
    SECTION("stmt loop back")
    {
        CHECK(cfg1->isStatementNext(4, 3));
        CHECK(cfg2->isStatementNext(4, 3));
        CHECK(cfg2->isStatementNext(6, 1));
        CHECK(cfg2->isStatementNext(7, 1));
        CHECK(cfg2->getNextStatements(7).size() == 1);
    }
    SECTION("while loop back")
    {
        CHECK(cfg1->isStatementNext(2, 1));
        CHECK(cfg1->isStatementNext(3, 2));
        CHECK(cfg2->isStatementNext(3, 1));
        CHECK(cfg2->getNextStatements(3).size() == 2);
    }
    SECTION("if -> stmt (in true and false body)")
    {
        CHECK(cfg1->isStatementNext(5, 6));
        CHECK(cfg1->isStatementNext(7, 8));
        CHECK(cfg1->isStatementNext(7, 9));
    }
    SECTION("if -> if")
    {
        CHECK(cfg1->isStatementNext(5, 7));
        CHECK(cfg2->isStatementNext(2, 5));
        CHECK(cfg1->getNextStatements(5).size() == 2);
    }
    SECTION("if -> while")
    {
        CHECK(cfg2->isStatementNext(2, 3));
    }
    SECTION("Negative test case: across diff proc")
    {
        CHECK(!cfg1->isStatementNext(4, 5));
        CHECK(!cfg2->isStatementNext(7, 8));
        CHECK(!cfg2->isStatementNext(12, 1));
    }
    SECTION("Negative test case: same proc but more than 1 level of nestings and checks for termination")
    {
        CHECK(!cfg1->isStatementNext(1, 3));
        CHECK(!cfg1->isStatementNext(2, 4));
        CHECK(!cfg1->isStatementNext(3, 1));
        CHECK(!cfg1->isStatementNext(4, 2));
        CHECK(!cfg1->isStatementNext(9, 7));
        CHECK(!cfg1->isStatementNext(8, 7));
        CHECK(!cfg1->isStatementNext(7, 5));
        CHECK(!cfg1->isStatementNext(6, 5));
        CHECK(!cfg2->isStatementNext(4, 2));
        CHECK(!cfg2->isStatementNext(4, 1));
        CHECK(!cfg2->isStatementNext(10, 16));
    }
    SECTION("Negative test case: same proc, same stmtList")
    {
        CHECK(!cfg2->isStatementNext(10, 19));
        CHECK(!cfg2->isStatementNext(19, 21));
    }
    SECTION("Negative test case: same stmt to same stmt ")
    {
        CHECK(!cfg1->isStatementNext(1, 1));
        CHECK(!cfg1->isStatementNext(2, 2));
        CHECK(!cfg1->isStatementNext(3, 3));
        CHECK(!cfg1->isStatementNext(4, 4));
        CHECK(!cfg1->isStatementNext(5, 5));
        CHECK(!cfg1->isStatementNext(6, 6));
        CHECK(!cfg1->isStatementNext(7, 7));
        CHECK(!cfg1->isStatementNext(9, 9));
    }
    SECTION("Fail from indices out of range")
    {
        CHECK_THROWS_WITH(cfg1->isStatementNext(1, 8000), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(cfg1->isStatementNext(-1, 5), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(cfg1->isStatementNext(0, 5), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(cfg1->getNextStatements(8000), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(cfg1->getNextStatements(-1), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(cfg1->getNextStatements(0), Catch::Matchers::Contains("Statement number out of range"));
    }
}

TEST_CASE("Next*(a,b)")
{
    SECTION("stmt->stmt")
    {
        CHECK(cfg1->isStatementTransitivelyNext(1, 2));
        CHECK(cfg1->isStatementTransitivelyNext(2, 3));
        CHECK(cfg2->isStatementTransitivelyNext(10, 16));
        CHECK(cfg1->isStatementTransitivelyNext(1, 1));
        CHECK(cfg1->isStatementTransitivelyNext(2, 2));
        CHECK(cfg1->isStatementTransitivelyNext(3, 3));
        CHECK(cfg1->isStatementTransitivelyNext(4, 4));
        CHECK(cfg2->getTransitivelyNextStatements(19).size() == 2);
    }
    SECTION("while->while for 1 or 2 levels of nesting")
    {
        CHECK(cfg1->isStatementTransitivelyNext(1, 2));
        CHECK(cfg1->isStatementTransitivelyNext(2, 3));
        CHECK(cfg1->isStatementTransitivelyNext(1, 3));
        CHECK(cfg1->isStatementTransitivelyNext(2, 4));
        CHECK(cfg1->isStatementTransitivelyNext(3, 1));
        CHECK(cfg1->isStatementTransitivelyNext(4, 2));
        CHECK(cfg2->isStatementTransitivelyNext(4, 2));
        CHECK(cfg2->isStatementTransitivelyNext(4, 1));
        CHECK(cfg1->getTransitivelyNextStatements(2).size() == 4);
    }
    SECTION("while->stmt")
    {
        CHECK(cfg1->isStatementTransitivelyNext(3, 4));
        CHECK(cfg2->isStatementTransitivelyNext(3, 4));
        CHECK(cfg2->getTransitivelyNextStatements(4).size() == 7);
    }
    SECTION("while->if")
    {
        CHECK(cfg2->isStatementTransitivelyNext(1, 2));
    }
    SECTION("while beside if")
    {
        CHECK(cfg2->isStatementTransitivelyNext(10, 14));
    }
    SECTION("stmt loop back")
    {
        CHECK(cfg1->isStatementTransitivelyNext(4, 3));
        CHECK(cfg2->isStatementTransitivelyNext(4, 3));
        CHECK(cfg2->isStatementTransitivelyNext(6, 1));
        CHECK(cfg2->isStatementTransitivelyNext(7, 1));
        CHECK(cfg2->getTransitivelyNextStatements(13).size() == 12);
    }
    SECTION("while loop back")
    {
        CHECK(cfg1->isStatementTransitivelyNext(2, 1));
        CHECK(cfg1->isStatementTransitivelyNext(3, 2));
        CHECK(cfg2->isStatementTransitivelyNext(3, 1));
        CHECK(cfg2->getTransitivelyNextStatements(3).size() == 7);
    }
    SECTION("if -> stmt (in true and false body)")
    {
        CHECK(cfg1->isStatementTransitivelyNext(5, 6));
        CHECK(cfg1->isStatementTransitivelyNext(7, 8));
        CHECK(cfg1->isStatementTransitivelyNext(7, 9));
    }
    SECTION("if -> if")
    {
        CHECK(cfg1->isStatementTransitivelyNext(5, 7));
        CHECK(cfg2->isStatementTransitivelyNext(2, 5));
        CHECK(cfg1->getTransitivelyNextStatements(5).size() == 4);
    }
    SECTION("if -> while")
    {
        CHECK(cfg2->isStatementTransitivelyNext(2, 3));
    }
    SECTION("Negative test case: across diff proc")
    {
        CHECK(!cfg1->isStatementTransitivelyNext(4, 5));
        CHECK(!cfg2->isStatementTransitivelyNext(7, 8));
        CHECK(!cfg2->isStatementTransitivelyNext(12, 1));
    }
    SECTION("Negative test case: same proc, diff stmtList and checks for termination")
    {
        CHECK(!cfg1->isStatementTransitivelyNext(9, 7));
        CHECK(!cfg1->isStatementTransitivelyNext(8, 7));
        CHECK(!cfg1->isStatementTransitivelyNext(7, 5));
        CHECK(!cfg1->isStatementTransitivelyNext(6, 5));
    }
    SECTION("same proc, same stmtList")
    {
        CHECK(cfg2->isStatementTransitivelyNext(10, 19));
        CHECK(cfg2->isStatementTransitivelyNext(19, 21));
    }
    SECTION("Negative test case: same stmt to same stmt ")
    {
        CHECK(!cfg1->isStatementTransitivelyNext(5, 5));
        CHECK(!cfg1->isStatementTransitivelyNext(6, 6));
        CHECK(!cfg1->isStatementTransitivelyNext(7, 7));
        CHECK(!cfg1->isStatementTransitivelyNext(9, 9));
    }
    SECTION("Fail from indices out of range")
    {
        CHECK_THROWS_WITH(
            cfg1->isStatementTransitivelyNext(1, 8000), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(
            cfg1->isStatementTransitivelyNext(-1, 5), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(
            cfg1->isStatementTransitivelyNext(0, 5), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(
            cfg1->getTransitivelyNextStatements(8000), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(
            cfg1->getTransitivelyNextStatements(-1), Catch::Matchers::Contains("Statement number out of range"));
        CHECK_THROWS_WITH(
            cfg1->getTransitivelyNextStatements(0), Catch::Matchers::Contains("Statement number out of range"));
    }
}

TEST_CASE("Affects(a,b)")
{
    SECTION("Straightforward")
    {
        CHECK(cfg2->doesAffect(8, 11));
        CHECK(cfg2->doesAffect(9, 13));
        CHECK(cfg2->doesAffect(19, 20));
        CHECK(cfg2->doesAffect(19, 21));
        CHECK(cfg2->doesAffect(20, 21));
    }

    SECTION("Prefer to skip while")
    {
        CHECK(cfg2->doesAffect(8, 15));
        CHECK(cfg2->doesAffect(9, 19));
    }

    SECTION("Prefer one branch")
    {
        CHECK(cfg3->doesAffect(1, 13));
        CHECK(cfg3->doesAffect(2, 13));
    }

    SECTION("Negative test case: modified by procedure call")
    {
        CHECK(!cfg2->doesAffect(11, 15));
    }

    SECTION("Negative test case: modified by read statement")
    {
        CHECK(!cfg3->doesAffect(4, 9));
    }

    SECTION("Negative test case: cannot skip over both branches")
    {
        CHECK(!cfg2->doesAffect(8, 19));
        CHECK(!cfg2->doesAffect(8, 21));
        CHECK(!cfg2->doesAffect(11, 19));
        CHECK(!cfg2->doesAffect(11, 21));
    }

    SECTION("Negative test case: not assign statements")
    {
        CHECK(!cfg1->doesAffect(1, 3));
        CHECK(!cfg1->doesAffect(1, 4));
        CHECK(!cfg2->doesAffect(4, 5));
        CHECK(!cfg2->doesAffect(14, 16));
    }

    SECTION("Negative test case: Next*(a, b) doesn't hold")
    {
        CHECK(!cfg1->doesAffect(5, 5));
        CHECK(!cfg1->doesAffect(6, 5));
        CHECK(!cfg2->doesAffect(6, 7));
        CHECK(!cfg2->doesAffect(21, 8));
    }
}

TEST_CASE("Affects*(a,b)")
{
    SECTION("Affects(a,b) holds")
    {
        CHECK(cfg2->doesTransitivelyAffect(8, 11));
        CHECK(cfg2->doesTransitivelyAffect(9, 13));
        CHECK(cfg4->doesTransitivelyAffect(1, 4));
        CHECK(cfg4->doesTransitivelyAffect(1, 10));
    }

    SECTION("One intermediary variable")
    {
        CHECK(cfg4->doesTransitivelyAffect(1, 11));
        CHECK(cfg4->doesTransitivelyAffect(1, 12));
    }

    SECTION("Two or more intermediary variables")
    {
        CHECK(cfg4->doesTransitivelyAffect(1, 13));
        CHECK(cfg4->doesTransitivelyAffect(1, 14));
    }

    SECTION("Negative test case: not assignment statements")
    {
        CHECK(!cfg3->doesTransitivelyAffect(1, 3));
        CHECK(!cfg3->doesTransitivelyAffect(3, 5));
        CHECK(!cfg4->doesTransitivelyAffect(6, 7));
    }

    SECTION("Negative test case: Next*(a,b) doesn't hold")
    {
        CHECK(!cfg3->doesTransitivelyAffect(3, 1));
        CHECK(!cfg4->doesTransitivelyAffect(8, 9));
    }
}
