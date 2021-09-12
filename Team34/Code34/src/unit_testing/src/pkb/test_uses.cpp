#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "simple/parser.h"
#include "pkb.h"
#include "pql/parser/parser.h"
#include "util.h"

using namespace simple::parser;
using namespace pkb;

static void req(bool b)
{
    REQUIRE(b);
}

// adapted from sample query. Changed line 8 to not use "x", 17 to not use "i" for condition checking
constexpr const auto sample_source = R"(
    procedure Example {
    x = 2;
    z = 3;
    i = 5;
    while (i!=0) {
    x = x - 1;
    if (x==1) then {
        z = x + 1; }
    else {
        y = z + 1; }
    z = z + x + i;
    call q;
    i = i - 1; }
    call p; }

    procedure p {
        if (x<0) then {
        while (i>0) {
            x = z * 3 + 2 * y;
            call q;
            i = y - 1; }
        x = x + 1;
        z = x + z; }
        else {
        z = 1; }
        z = z + x + i; }

    procedure q {
        if (x==1) then {
        z = x + 1; }
        else {
        x = z + x; } }
)";

constexpr const auto trivial_source = R"(
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

auto prog1 = parseProgram(sample_source).unwrap();
auto kb_sample = processProgram(prog1).unwrap();

auto prog2 = parseProgram(trivial_source).unwrap();
auto kb_trivial = processProgram(prog2).unwrap();

TEST_CASE("Uses(DeclaredStmt, DeclaredVar)")
{
    SECTION("Uses(DeclaredStmt, DeclaredVar) for assignment, condition and print")
    {
        req(kb_sample->uses_modifies.isUses(4, "i"));
        req(kb_sample->uses_modifies.isUses(6, "x"));
        req(kb_sample->uses_modifies.isUses(9, "i"));
        req(kb_sample->uses_modifies.isUses(9, "x"));
        req(kb_sample->uses_modifies.isUses(9, "z"));
        req(kb_sample->uses_modifies.isUses(13, "x"));
        req(kb_sample->uses_modifies.isUses(14, "i"));

        req(kb_trivial->uses_modifies.isUses(6, "flag"));
        req(kb_trivial->uses_modifies.isUses(9, "normSq"));
        req(kb_trivial->uses_modifies.isUses(14, "x"));
        req(kb_trivial->uses_modifies.isUses(14, "y"));
        req(kb_trivial->uses_modifies.isUses(19, "count"));
        req(kb_trivial->uses_modifies.isUses(23, "cenX"));
        req(kb_trivial->uses_modifies.isUses(23, "cenY"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) inside if/while")
    {
        req(kb_sample->uses_modifies.isUses(4, "x"));
        req(kb_sample->uses_modifies.isUses(4, "z"));
        req(kb_sample->uses_modifies.isUses(6, "x"));
        req(kb_sample->uses_modifies.isUses(6, "z"));
        req(kb_sample->uses_modifies.isUses(13, "i"));
        req(kb_sample->uses_modifies.isUses(13, "x"));
        req(kb_sample->uses_modifies.isUses(13, "y"));
        req(kb_sample->uses_modifies.isUses(13, "z"));

        req(kb_trivial->uses_modifies.isUses(14, "cenX"));
        req(kb_trivial->uses_modifies.isUses(14, "cenY"));
        req(kb_trivial->uses_modifies.isUses(14, "count"));
        req(kb_trivial->uses_modifies.isUses(14, "x"));
        req(kb_trivial->uses_modifies.isUses(14, "y"));
        req(kb_trivial->uses_modifies.isUses(19, "cenX"));
        req(kb_trivial->uses_modifies.isUses(19, "cenY"));
        req(kb_trivial->uses_modifies.isUses(19, "count"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) for procCall")
    {
        req(kb_sample->uses_modifies.isUses(10, "x"));
        req(kb_sample->uses_modifies.isUses(10, "z"));
        req(kb_sample->uses_modifies.isUses(12, "i"));
        req(kb_sample->uses_modifies.isUses(12, "x"));
        req(kb_sample->uses_modifies.isUses(12, "y"));
        req(kb_sample->uses_modifies.isUses(12, "z"));

        req(kb_trivial->uses_modifies.isUses(2, "cenX"));
        req(kb_trivial->uses_modifies.isUses(2, "cenY"));
        req(kb_trivial->uses_modifies.isUses(2, "count"));
        req(kb_trivial->uses_modifies.isUses(2, "x"));
        req(kb_trivial->uses_modifies.isUses(2, "y"));
        req(kb_trivial->uses_modifies.isUses(3, "cenX"));
        req(kb_trivial->uses_modifies.isUses(3, "cenY"));
        req(kb_trivial->uses_modifies.isUses(3, "flag"));
        req(kb_trivial->uses_modifies.isUses(3, "normSq"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) negative test cases")
    {
        req(!kb_sample->uses_modifies.isUses(3, "i"));
        req(!kb_sample->uses_modifies.isUses(6, "y"));
        req(!kb_sample->uses_modifies.isUses(7, "z"));
        req(!kb_sample->uses_modifies.isUses(10, "i"));


        req(!kb_trivial->uses_modifies.isUses(1, "flag"));
        req(!kb_trivial->uses_modifies.isUses(14, "flag"));
        req(!kb_trivial->uses_modifies.isUses(18, "x"));
        req(!kb_trivial->uses_modifies.isUses(18, "y"));
    }
}

TEST_CASE("Uses(DeclaredProc, DeclaredVar)")
{
    SECTION("Uses(DeclaredProc, DeclaredVar)")
    {
        req(kb_sample->uses_modifies.isUses("Example", "i"));
        req(kb_sample->uses_modifies.isUses("Example", "x"));
        req(kb_sample->uses_modifies.isUses("Example", "y"));
        req(kb_sample->uses_modifies.isUses("Example", "z"));
        req(kb_sample->uses_modifies.isUses("p", "i"));
        req(kb_sample->uses_modifies.isUses("p", "x"));
        req(kb_sample->uses_modifies.isUses("p", "y"));
        req(kb_sample->uses_modifies.isUses("p", "z"));
        req(kb_sample->uses_modifies.isUses("q", "x"));
        req(kb_sample->uses_modifies.isUses("q", "z"));

        req(kb_trivial->uses_modifies.isUses("main", "cenX"));
        req(kb_trivial->uses_modifies.isUses("main", "cenY"));
        req(kb_trivial->uses_modifies.isUses("main", "count"));
        req(kb_trivial->uses_modifies.isUses("main", "flag"));
        req(kb_trivial->uses_modifies.isUses("main", "normSq"));
        req(kb_trivial->uses_modifies.isUses("main", "x"));
        req(kb_trivial->uses_modifies.isUses("main", "y"));
        req(kb_trivial->uses_modifies.isUses("printResults", "cenX"));
        req(kb_trivial->uses_modifies.isUses("printResults", "cenY"));
        req(kb_trivial->uses_modifies.isUses("printResults", "flag"));
        req(kb_trivial->uses_modifies.isUses("printResults", "normSq"));
        req(kb_trivial->uses_modifies.isUses("computeCentroid", "cenX"));
        req(kb_trivial->uses_modifies.isUses("computeCentroid", "cenY"));
        req(kb_trivial->uses_modifies.isUses("computeCentroid", "count"));
        req(kb_trivial->uses_modifies.isUses("computeCentroid", "x"));
        req(kb_trivial->uses_modifies.isUses("computeCentroid", "y"));
    }

    SECTION("Uses(DeclaredProc, DeclaredVar) negative test cases")
    {
        req(!kb_sample->uses_modifies.isUses("q", "y"));
        req(!kb_sample->uses_modifies.isUses("q", "i"));


        req(!kb_trivial->uses_modifies.isUses("printResults", "x"));
        req(!kb_trivial->uses_modifies.isUses("printResults", "y"));
        req(!kb_trivial->uses_modifies.isUses("computeCentroid", "flag"));
        req(!kb_trivial->uses_modifies.isUses("computeCentroid", "normSq"));
    }
}

TEST_CASE("Uses(DeclaredStmt, AllVar)")
{
    SECTION("Uses(DeclaredStmt, AllVar) for assignment and print")
    {
        auto fst_result = kb_sample->uses_modifies.getUsesVars(1);
        req(fst_result.size() == 0);

        auto snd_result = kb_sample->uses_modifies.getUsesVars(5);
        req(snd_result.size() == 1);
        req(snd_result.count("x"));

        auto trd_result = kb_trivial->uses_modifies.getUsesVars(6);
        req(trd_result.size() == 1);
        req(trd_result.count("flag"));
    }

    SECTION("Uses(DeclaredStmt, AllVar) inside if/while")
    {
        auto fst_result = kb_sample->uses_modifies.getUsesVars(6);
        req(fst_result.size() == 2);
        req(fst_result.count("x"));
        req(fst_result.count("z"));

        auto snd_result = kb_sample->uses_modifies.getUsesVars(14);
        req(snd_result.size() == 4);
        req(snd_result.count("i"));
        req(snd_result.count("x"));
        req(snd_result.count("y"));
        req(snd_result.count("z"));
    }

    SECTION("Uses(DeclaredStmt, AllVar) for procCalls")
    {
        auto fst_result = kb_trivial->uses_modifies.getUsesVars(3);
        req(fst_result.size() == 4);
        req(fst_result.count("cenX"));
        req(fst_result.count("cenY"));
        req(fst_result.count("flag"));
        req(fst_result.count("normSq"));

        auto snd_result = kb_sample->uses_modifies.getUsesVars(10);
        req(snd_result.size() == 2);
        req(snd_result.count("x"));
        req(snd_result.count("z"));
    }
}

TEST_CASE("Uses(DeclaredProc, AllVar)")
{
    SECTION("Uses(DeclaredProc, AllVar) for assignment and print")
    {
        auto fst_result = kb_trivial->uses_modifies.getUsesVars("main");
        req(fst_result.size() == 7);
        req(fst_result.count("cenX"));
        req(fst_result.count("cenY"));
        req(fst_result.count("count"));
        req(fst_result.count("flag"));
        req(fst_result.count("normSq"));
        req(fst_result.count("x"));
        req(fst_result.count("y"));

        auto snd_result = kb_trivial->uses_modifies.getUsesVars("readPoint");
        req(snd_result.size() == 0);

        auto trd_result = kb_trivial->uses_modifies.getUsesVars("computeCentroid");
        req(trd_result.size() == 5);
        req(trd_result.count("cenX"));
        req(trd_result.count("cenY"));
        req(trd_result.count("count"));
        req(trd_result.count("x"));
        req(trd_result.count("y"));

        auto frh_result = kb_trivial->uses_modifies.getUsesVars("printResults");
        req(frh_result.size() == 4);
        req(frh_result.count("cenX"));
        req(frh_result.count("cenY"));
        req(frh_result.count("flag"));
        req(frh_result.count("normSq"));
    }
}

TEST_CASE("Uses(Synonym, DeclaredVar)")
{
    SECTION("Uses(ASSIGN, DeclaredVar)")
    {
        auto fst_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::ASSIGN, "count");
        req(fst_result.size() == 3);
        req(fst_result.count("15"));
        req(fst_result.count("21"));
        req(fst_result.count("22"));

        auto snd_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::ASSIGN, "cenX");
        req(snd_result.size() == 3);
        req(snd_result.count("16"));
        req(snd_result.count("21"));
        req(snd_result.count("23"));
    }

    SECTION("Uses(PRINT, DeclaredVar)")
    {
        auto fst_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::PRINT, "flag");
        req(fst_result.size() == 1);
        req(fst_result.count("6"));

        auto snd_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::PRINT, "cenX");
        req(snd_result.size() == 1);
        req(snd_result.count("7"));
    }

    SECTION("Uses(CALL, DeclaredVar)")
    {
        auto fst_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::CALL, "flag");
        req(fst_result.size() == 1);
        req(fst_result.count("3"));

        auto snd_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::CALL, "cenX");
        req(snd_result.size() == 2);
        req(snd_result.count("2"));
        req(snd_result.count("3"));
    }

    SECTION("Uses(IF, DeclaredVar)")
    {
        auto fst_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::IF, "x");
        req(fst_result.size() == 3);
        req(fst_result.count("6"));
        req(fst_result.count("13"));
        req(fst_result.count("22"));

        auto snd_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::IF, "i");
        req(snd_result.size() == 1);
        req(snd_result.count("13"));
    }

    SECTION("Uses(WHILE, DeclaredVar)")
    {
        auto fst_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::WHILE, "y");
        req(fst_result.size() == 1);
        req(fst_result.count("14"));

        auto snd_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::WHILE, "i");
        req(snd_result.size() == 2);
        req(snd_result.count("4"));
        req(snd_result.count("14"));
    }

    SECTION("Uses(STMT, DeclaredVar)")
    {
        auto fst_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::STMT, "x");
        req(fst_result.size() == 16);
        req(fst_result.count("4"));
        req(fst_result.count("5"));
        req(fst_result.count("6"));
        req(fst_result.count("7"));
        req(fst_result.count("9"));
        req(fst_result.count("10"));
        req(fst_result.count("12"));
        req(fst_result.count("13"));
        req(fst_result.count("14"));
        req(fst_result.count("16"));
        req(fst_result.count("18"));
        req(fst_result.count("19"));
        req(fst_result.count("21"));
        req(fst_result.count("22"));
        req(fst_result.count("23"));
        req(fst_result.count("24"));

        auto snd_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::STMT, "x");
        req(snd_result.size() == 3);
        req(snd_result.count("2"));
        req(snd_result.count("14"));
        req(snd_result.count("16"));
    }

    SECTION("Uses(PROCEDURE, DeclaredVar)")
    {
        auto fst_result = kb_sample->uses_modifies.getUses(pql::ast::DESIGN_ENT::PROCEDURE, "y");
        req(fst_result.size() == 2);
        req(fst_result.count("Example"));
        req(fst_result.count("p"));

        auto snd_result = kb_trivial->uses_modifies.getUses(pql::ast::DESIGN_ENT::PROCEDURE, "flag");
        req(snd_result.size() == 2);
        req(snd_result.count("printResults"));
        req(snd_result.count("main"));
    }
}
