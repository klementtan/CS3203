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

auto prog1_m = parseProgram(sample_source).unwrap();
auto kb_sample_m = processProgram(prog1_m).unwrap();

auto prog2_m = parseProgram(trivial_source).unwrap();
auto kb_trivial_m = processProgram(prog2_m).unwrap();

TEST_CASE("Modifies(DeclaredStmt, DeclaredVar)")
{
    SECTION("Modifies(DeclaredStmt, DeclaredVar) for assignment, condition and read")
    {
        req(kb_sample_m->uses_modifies.isModifies(1, "x"));
        req(kb_sample_m->uses_modifies.isModifies(5, "x"));
        req(kb_sample_m->uses_modifies.isModifies(8, "y"));
        req(kb_sample_m->uses_modifies.isModifies(9, "z"));

        req(kb_trivial_m->uses_modifies.isModifies(4, "x"));
        req(kb_trivial_m->uses_modifies.isModifies(5, "y"));
        req(kb_trivial_m->uses_modifies.isModifies(15, "count"));
        req(kb_trivial_m->uses_modifies.isModifies(20, "flag"));
        req(kb_trivial_m->uses_modifies.isModifies(21, "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies(23, "normSq"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) inside if/while")
    {
        req(kb_sample_m->uses_modifies.isModifies(4, "x"));
        req(kb_sample_m->uses_modifies.isModifies(4, "y"));
        req(kb_sample_m->uses_modifies.isModifies(4, "z"));
        req(kb_sample_m->uses_modifies.isModifies(6, "y"));
        req(kb_sample_m->uses_modifies.isModifies(6, "z"));
        req(kb_sample_m->uses_modifies.isModifies(13, "i"));
        req(kb_sample_m->uses_modifies.isModifies(13, "x"));
        req(kb_sample_m->uses_modifies.isModifies(13, "z"));

        req(kb_trivial_m->uses_modifies.isModifies(14, "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies(14, "cenY"));
        req(kb_trivial_m->uses_modifies.isModifies(14, "count"));
        req(kb_trivial_m->uses_modifies.isModifies(19, "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies(19, "cenY"));
        req(kb_trivial_m->uses_modifies.isModifies(19, "flag"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) for procCall")
    {
        req(kb_sample_m->uses_modifies.isModifies(10, "x"));
        req(kb_sample_m->uses_modifies.isModifies(10, "z"));
        req(kb_sample_m->uses_modifies.isModifies(12, "i"));
        req(kb_sample_m->uses_modifies.isModifies(12, "x"));
        req(kb_sample_m->uses_modifies.isModifies(12, "z"));

        req(kb_trivial_m->uses_modifies.isModifies(2, "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies(2, "cenY"));
        req(kb_trivial_m->uses_modifies.isModifies(2, "count"));
        req(kb_trivial_m->uses_modifies.isModifies(2, "x"));
        req(kb_trivial_m->uses_modifies.isModifies(2, "y"));
        req(kb_trivial_m->uses_modifies.isModifies(18, "x"));
        req(kb_trivial_m->uses_modifies.isModifies(18, "y"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) negative test cases")
    {
        req(!kb_sample_m->uses_modifies.isModifies(6, "x"));
        req(!kb_sample_m->uses_modifies.isModifies(7, "x"));
        req(!kb_sample_m->uses_modifies.isModifies(15, "z"));
        req(!kb_sample_m->uses_modifies.isModifies(16, "i"));

        req(!kb_trivial_m->uses_modifies.isModifies(6, "flag"));
        req(!kb_trivial_m->uses_modifies.isModifies(14, "flag"));
        req(!kb_trivial_m->uses_modifies.isModifies(23, "cenX"));
        req(!kb_trivial_m->uses_modifies.isModifies(23, "cenY"));
    }
}

TEST_CASE("Modifies(DeclaredProc, DeclaredVar)")
{
    SECTION("Modifies(DeclaredProc, DeclaredVar)")
    {
        req(kb_sample_m->uses_modifies.isModifies("Example", "i"));
        req(kb_sample_m->uses_modifies.isModifies("Example", "x"));
        req(kb_sample_m->uses_modifies.isModifies("Example", "y"));
        req(kb_sample_m->uses_modifies.isModifies("Example", "z"));
        req(kb_sample_m->uses_modifies.isModifies("p", "i"));
        req(kb_sample_m->uses_modifies.isModifies("p", "x"));
        req(kb_sample_m->uses_modifies.isModifies("p", "z"));
        req(kb_sample_m->uses_modifies.isModifies("q", "x"));
        req(kb_sample_m->uses_modifies.isModifies("q", "z"));

        req(kb_trivial_m->uses_modifies.isModifies("main", "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "cenY"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "count"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "flag"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "normSq"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "x"));
        req(kb_trivial_m->uses_modifies.isModifies("main", "y"));
        req(kb_trivial_m->uses_modifies.isModifies("readPoint", "x"));
        req(kb_trivial_m->uses_modifies.isModifies("readPoint", "y"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "cenX"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "cenY"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "count"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "flag"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "normSq"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "x"));
        req(kb_trivial_m->uses_modifies.isModifies("computeCentroid", "y"));
    }

    SECTION("Modifies(DeclaredProc, DeclaredVar) negative test cases")
    {
        req(!kb_sample_m->uses_modifies.isModifies("p", "y"));
        req(!kb_sample_m->uses_modifies.isModifies("q", "y"));
        req(!kb_sample_m->uses_modifies.isModifies("q", "i"));


        req(!kb_trivial_m->uses_modifies.isModifies("readPoint", "cenX"));
        req(!kb_trivial_m->uses_modifies.isModifies("readPoint", "cenY"));
        req(!kb_trivial_m->uses_modifies.isModifies("printResults", "cenX"));
        req(!kb_trivial_m->uses_modifies.isModifies("printResults", "cenY"));
    }
}

TEST_CASE("Modifies(DeclaredStmt, AllVar)")
{
    SECTION("Modifies(DeclaredStmt, AllVar) for assignment and read")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifiesVars(1);
        req(fst_result.size() == 1);
        req(fst_result.count("x"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifiesVars(4);
        req(snd_result.size() == 1);
        req(snd_result.count("x"));
    }

    SECTION("Modifies(DeclaredStmt, AllVar) inside if/while")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifiesVars(4);
        req(fst_result.size() == 4);
        req(fst_result.count("i"));
        req(fst_result.count("x"));
        req(fst_result.count("y"));
        req(fst_result.count("z"));

        auto snd_result = kb_sample_m->uses_modifies.getModifiesVars(6);
        req(snd_result.size() == 2);
        req(snd_result.count("y"));
        req(snd_result.count("z"));
    }

    SECTION("Modifies(DeclaredStmt, AllVar) for procCalls")
    {
        auto fst_result = kb_trivial_m->uses_modifies.getModifiesVars(2);
        req(fst_result.size() == 7);
        req(fst_result.count("cenX"));
        req(fst_result.count("cenY"));
        req(fst_result.count("count"));
        req(fst_result.count("flag"));
        req(fst_result.count("normSq"));
        req(fst_result.count("x"));
        req(fst_result.count("y"));

        auto snd_result = kb_sample_m->uses_modifies.getModifiesVars(10);
        req(snd_result.size() == 2);
        req(snd_result.count("x"));
        req(snd_result.count("z"));

        auto trd_result = kb_trivial_m->uses_modifies.getModifiesVars(18);
        req(trd_result.size() == 2);
        req(trd_result.count("x"));
        req(trd_result.count("y"));
    }
}

TEST_CASE("Modifies(DeclaredProc, AllVar)")
{
    SECTION("Modifies(DeclaredProc, AllVar) for assignment and print")
    {
        auto fst_result = kb_trivial_m->uses_modifies.getModifiesVars("main");
        req(fst_result.size() == 7);
        req(fst_result.count("cenX"));
        req(fst_result.count("cenY"));
        req(fst_result.count("count"));
        req(fst_result.count("flag"));
        req(fst_result.count("normSq"));
        req(fst_result.count("x"));
        req(fst_result.count("y"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifiesVars("readPoint");
        req(snd_result.size() == 2);
        req(snd_result.count("x"));
        req(snd_result.count("y"));

        auto trd_result = kb_trivial_m->uses_modifies.getModifiesVars("computeCentroid");
        req(trd_result.size() == 7);
        req(trd_result.count("cenX"));
        req(trd_result.count("cenY"));
        req(trd_result.count("count"));
        req(trd_result.count("flag"));
        req(trd_result.count("normSq"));
        req(trd_result.count("x"));
        req(trd_result.count("y"));

        auto frh_result = kb_trivial_m->uses_modifies.getModifiesVars("printResults");
        req(frh_result.size() == 0);
    }
}

TEST_CASE("Modifies(Synonym, DeclaredVar)")
{
    SECTION("Modifies(ASSIGN, DeclaredVar)")
    {
        auto fst_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::ASSIGN, "flag");
        req(fst_result.size() == 2);
        req(fst_result.count("1"));
        req(fst_result.count("20"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::ASSIGN, "cenX");
        req(snd_result.size() == 3);
        req(snd_result.count("11"));
        req(snd_result.count("16"));
        req(snd_result.count("21"));
    }

    SECTION("Modifies(READ, DeclaredVar)")
    {
        auto fst_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::READ, "x");
        req(fst_result.size() == 1);
        req(fst_result.count("4"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::READ, "y");
        req(snd_result.size() == 1);
        req(snd_result.count("5"));
    }

    SECTION("Modifies(CALL, DeclaredVar)")
    {
        auto fst_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::CALL, "flag");
        req(fst_result.size() == 1);
        req(fst_result.count("2"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::CALL, "x");
        req(snd_result.size() == 3);
        req(snd_result.count("2"));
        req(snd_result.count("13"));
        req(snd_result.count("18"));
    }

    SECTION("Modifies(IF, DeclaredVar)")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::IF, "x");
        req(fst_result.size() == 2);
        req(fst_result.count("13"));
        req(fst_result.count("22"));

        auto snd_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::IF, "i");
        req(snd_result.size() == 1);
        req(snd_result.count("13"));
    }

    SECTION("Modifies(WHILE, DeclaredVar)")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::WHILE, "y");
        req(fst_result.size() == 1);
        req(fst_result.count("4"));

        auto snd_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::WHILE, "i");
        req(snd_result.size() == 2);
        req(snd_result.count("4"));
        req(snd_result.count("14"));
    }

    SECTION("Uses(STMT, DeclaredVar)")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::STMT, "x");
        req(fst_result.size() == 12);
        req(fst_result.count("1"));
        req(fst_result.count("4"));
        req(fst_result.count("5"));
        req(fst_result.count("10"));
        req(fst_result.count("12"));
        req(fst_result.count("13"));
        req(fst_result.count("14"));
        req(fst_result.count("15"));
        req(fst_result.count("16"));
        req(fst_result.count("18"));
        req(fst_result.count("22"));
        req(fst_result.count("24"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::STMT, "x");
        req(snd_result.size() == 5);
        req(snd_result.count("2"));
        req(snd_result.count("4"));
        req(snd_result.count("13"));
        req(snd_result.count("14"));
        req(snd_result.count("18"));
    }

    SECTION("Uses(PROCEDURE, DeclaredVar)")
    {
        auto fst_result = kb_sample_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::PROCEDURE, "y");
        req(fst_result.size() == 1);
        req(fst_result.count("Example"));

        auto snd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::PROCEDURE, "flag");
        req(snd_result.size() == 2);
        req(snd_result.count("computeCentroid"));
        req(snd_result.count("main"));

        auto trd_result = kb_trivial_m->uses_modifies.getModifies(pql::ast::DESIGN_ENT::PROCEDURE, "x");
        req(trd_result.size() == 3);
        req(trd_result.count("computeCentroid"));
        req(trd_result.count("main"));
        req(trd_result.count("readPoint"));
    }
}
