#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "simple/parser.h"
#include "pkb.h"
#include "design_extractor.h"
#include "pql/parser/parser.h"
#include "util.h"

using namespace simple::parser;
using namespace pkb;

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

static auto kb_sample = DesignExtractor(parseProgram(sample_source)).run();
static auto kb_trivial = DesignExtractor(parseProgram(trivial_source)).run();

static const Statement& get_stmt(const std::unique_ptr<pkb::ProgramKB>& pkb, simple::ast::StatementNum num)
{
    return pkb->getStatementAt(num);
}

static const Procedure& get_proc(const std::unique_ptr<pkb::ProgramKB>& pkb, const char* name)
{
    return pkb->getProcedureNamed(name);
}

static const Variable& get_var(const std::unique_ptr<pkb::ProgramKB>& pkb, const char* name)
{
    return pkb->getVariableNamed(name);
}


TEST_CASE("Modifies(DeclaredStmt, DeclaredVar)")
{
    SECTION("Modifies(DeclaredStmt, DeclaredVar) for assignment, condition and read")
    {
        CHECK(get_stmt(kb_sample, 1).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 5).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 8).modifiesVariable("y"));
        CHECK(get_stmt(kb_sample, 9).modifiesVariable("z"));

        CHECK(get_stmt(kb_trivial, 4).modifiesVariable("x"));
        CHECK(get_stmt(kb_trivial, 5).modifiesVariable("y"));
        CHECK(get_stmt(kb_trivial, 15).modifiesVariable("count"));
        CHECK(get_stmt(kb_trivial, 20).modifiesVariable("flag"));
        CHECK(get_stmt(kb_trivial, 21).modifiesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 23).modifiesVariable("normSq"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) inside if/while")
    {
        CHECK(get_stmt(kb_sample, 4).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 4).modifiesVariable("y"));
        CHECK(get_stmt(kb_sample, 4).modifiesVariable("z"));
        CHECK(get_stmt(kb_sample, 6).modifiesVariable("y"));
        CHECK(get_stmt(kb_sample, 6).modifiesVariable("z"));
        CHECK(get_stmt(kb_sample, 13).modifiesVariable("i"));
        CHECK(get_stmt(kb_sample, 13).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 13).modifiesVariable("z"));

        CHECK(get_stmt(kb_trivial, 14).modifiesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 14).modifiesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 14).modifiesVariable("count"));
        CHECK(get_stmt(kb_trivial, 19).modifiesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 19).modifiesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 19).modifiesVariable("flag"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) for procCall")
    {
        CHECK(get_stmt(kb_sample, 10).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 10).modifiesVariable("z"));
        CHECK(get_stmt(kb_sample, 12).modifiesVariable("i"));
        CHECK(get_stmt(kb_sample, 12).modifiesVariable("x"));
        CHECK(get_stmt(kb_sample, 12).modifiesVariable("z"));

        CHECK(get_stmt(kb_trivial, 2).modifiesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 2).modifiesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 2).modifiesVariable("count"));
        CHECK(get_stmt(kb_trivial, 2).modifiesVariable("x"));
        CHECK(get_stmt(kb_trivial, 2).modifiesVariable("y"));
        CHECK(get_stmt(kb_trivial, 18).modifiesVariable("x"));
        CHECK(get_stmt(kb_trivial, 18).modifiesVariable("y"));
    }

    SECTION("Modifies(DeclaredStmt, DeclaredVar) negative test cases")
    {
        CHECK_FALSE(get_stmt(kb_sample, 6).modifiesVariable("x"));
        CHECK_FALSE(get_stmt(kb_sample, 7).modifiesVariable("x"));
        CHECK_FALSE(get_stmt(kb_sample, 15).modifiesVariable("z"));
        CHECK_FALSE(get_stmt(kb_sample, 16).modifiesVariable("i"));

        CHECK_FALSE(get_stmt(kb_trivial, 6).modifiesVariable("flag"));
        CHECK_FALSE(get_stmt(kb_trivial, 14).modifiesVariable("flag"));
        CHECK_FALSE(get_stmt(kb_trivial, 23).modifiesVariable("cenX"));
        CHECK_FALSE(get_stmt(kb_trivial, 23).modifiesVariable("cenY"));
    }
}

TEST_CASE("Modifies(DeclaredProc, DeclaredVar)")
{
    SECTION("Modifies(DeclaredProc, DeclaredVar)")
    {
        CHECK(get_proc(kb_sample, "Example").modifiesVariable("i"));
        CHECK(get_proc(kb_sample, "Example").modifiesVariable("x"));
        CHECK(get_proc(kb_sample, "Example").modifiesVariable("y"));
        CHECK(get_proc(kb_sample, "Example").modifiesVariable("z"));
        CHECK(get_proc(kb_sample, "p").modifiesVariable("i"));
        CHECK(get_proc(kb_sample, "p").modifiesVariable("x"));
        CHECK(get_proc(kb_sample, "p").modifiesVariable("z"));
        CHECK(get_proc(kb_sample, "q").modifiesVariable("x"));
        CHECK(get_proc(kb_sample, "q").modifiesVariable("z"));

        CHECK(get_proc(kb_trivial, "main").modifiesVariable("cenX"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("cenY"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("count"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("flag"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("normSq"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("x"));
        CHECK(get_proc(kb_trivial, "main").modifiesVariable("y"));
        CHECK(get_proc(kb_trivial, "readPoint").modifiesVariable("x"));
        CHECK(get_proc(kb_trivial, "readPoint").modifiesVariable("y"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("cenX"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("cenY"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("count"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("flag"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("normSq"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("x"));
        CHECK(get_proc(kb_trivial, "computeCentroid").modifiesVariable("y"));
    }

    SECTION("Modifies(DeclaredProc, DeclaredVar) negative test cases")
    {
        CHECK_FALSE(get_proc(kb_sample, "p").modifiesVariable("y"));
        CHECK_FALSE(get_proc(kb_sample, "q").modifiesVariable("y"));
        CHECK_FALSE(get_proc(kb_sample, "q").modifiesVariable("i"));


        CHECK_FALSE(get_proc(kb_trivial, "readPoint").modifiesVariable("cenX"));
        CHECK_FALSE(get_proc(kb_trivial, "readPoint").modifiesVariable("cenY"));
        CHECK_FALSE(get_proc(kb_trivial, "printResults").modifiesVariable("cenX"));
        CHECK_FALSE(get_proc(kb_trivial, "printResults").modifiesVariable("cenY"));
    }
}

TEST_CASE("Modifies(DeclaredStmt, AllVar)")
{
    SECTION("Modifies(DeclaredStmt, AllVar) for assignment and read")
    {
        auto& fst_result = get_stmt(kb_sample, 1).getModifiedVariables();
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count("x"));

        auto& snd_result = get_stmt(kb_trivial, 4).getModifiedVariables();
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count("x"));
    }

    SECTION("Modifies(DeclaredStmt, AllVar) inside if/while")
    {
        auto& fst_result = get_stmt(kb_sample, 4).getModifiedVariables();
        CHECK(fst_result.size() == 4);
        CHECK(fst_result.count("i"));
        CHECK(fst_result.count("x"));
        CHECK(fst_result.count("y"));
        CHECK(fst_result.count("z"));

        auto& snd_result = get_stmt(kb_sample, 6).getModifiedVariables();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("y"));
        CHECK(snd_result.count("z"));
    }

    SECTION("Modifies(DeclaredStmt, AllVar) for procCalls")
    {
        auto& fst_result = get_stmt(kb_trivial, 2).getModifiedVariables();
        CHECK(fst_result.size() == 7);
        CHECK(fst_result.count("cenX"));
        CHECK(fst_result.count("cenY"));
        CHECK(fst_result.count("count"));
        CHECK(fst_result.count("flag"));
        CHECK(fst_result.count("normSq"));
        CHECK(fst_result.count("x"));
        CHECK(fst_result.count("y"));

        auto& snd_result = get_stmt(kb_sample, 10).getModifiedVariables();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("x"));
        CHECK(snd_result.count("z"));

        auto& trd_result = get_stmt(kb_trivial, 18).getModifiedVariables();
        CHECK(trd_result.size() == 2);
        CHECK(trd_result.count("x"));
        CHECK(trd_result.count("y"));
    }
}

TEST_CASE("Modifies(DeclaredProc, AllVar)")
{
    SECTION("Modifies(DeclaredProc, AllVar) for assignment and print")
    {
        auto& fst_result = get_proc(kb_trivial, "main").getModifiedVariables();
        CHECK(fst_result.size() == 7);
        CHECK(fst_result.count("cenX"));
        CHECK(fst_result.count("cenY"));
        CHECK(fst_result.count("count"));
        CHECK(fst_result.count("flag"));
        CHECK(fst_result.count("normSq"));
        CHECK(fst_result.count("x"));
        CHECK(fst_result.count("y"));

        auto& snd_result = get_proc(kb_trivial, "readPoint").getModifiedVariables();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("x"));
        CHECK(snd_result.count("y"));

        auto& trd_result = get_proc(kb_trivial, "computeCentroid").getModifiedVariables();
        CHECK(trd_result.size() == 7);
        CHECK(trd_result.count("cenX"));
        CHECK(trd_result.count("cenY"));
        CHECK(trd_result.count("count"));
        CHECK(trd_result.count("flag"));
        CHECK(trd_result.count("normSq"));
        CHECK(trd_result.count("x"));
        CHECK(trd_result.count("y"));

        auto& frh_result = get_proc(kb_trivial, "printResults").getModifiedVariables();
        CHECK(frh_result.size() == 0);
    }
}

TEST_CASE("Modifies(Synonym, DeclaredVar)")
{
    SECTION("Modifies(ASSIGN, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "flag").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::ASSIGN);
        CHECK(fst_result.size() == 2);
        CHECK(fst_result.count(1));
        CHECK(fst_result.count(20));

        auto snd_result = get_var(kb_trivial, "cenX").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::ASSIGN);
        CHECK(snd_result.size() == 3);
        CHECK(snd_result.count(11));
        CHECK(snd_result.count(16));
        CHECK(snd_result.count(21));
    }

    SECTION("Modifies(READ, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "x").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::READ);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(4));

        auto snd_result = get_var(kb_trivial, "y").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::READ);
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count(5));
    }

    SECTION("Modifies(CALL, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "flag").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::CALL);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(2));

        auto snd_result = get_var(kb_trivial, "x").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::CALL);
        CHECK(snd_result.size() == 3);
        CHECK(snd_result.count(2));
        CHECK(snd_result.count(13));
        CHECK(snd_result.count(18));
    }

    SECTION("Modifies(IF, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "x").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::IF);
        CHECK(fst_result.size() == 2);
        CHECK(fst_result.count(13));
        CHECK(fst_result.count(22));

        auto snd_result = get_var(kb_sample, "i").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::IF);
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count(13));
    }

    SECTION("Modifies(WHILE, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "y").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::WHILE);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(4));

        auto snd_result = get_var(kb_sample, "i").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::WHILE);
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count(4));
        CHECK(snd_result.count(14));
    }

    SECTION("Uses(STMT, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "x").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::STMT);
        CHECK(fst_result.size() == 12);
        CHECK(fst_result.count(1));
        CHECK(fst_result.count(4));
        CHECK(fst_result.count(5));
        CHECK(fst_result.count(10));
        CHECK(fst_result.count(12));
        CHECK(fst_result.count(13));
        CHECK(fst_result.count(14));
        CHECK(fst_result.count(15));
        CHECK(fst_result.count(16));
        CHECK(fst_result.count(18));
        CHECK(fst_result.count(22));
        CHECK(fst_result.count(24));

        auto snd_result = get_var(kb_trivial, "x").getModifyingStmtNumsFiltered(pql::ast::DESIGN_ENT::STMT);
        CHECK(snd_result.size() == 5);
        CHECK(snd_result.count(2));
        CHECK(snd_result.count(4));
        CHECK(snd_result.count(13));
        CHECK(snd_result.count(14));
        CHECK(snd_result.count(18));
    }

    SECTION("Uses(PROCEDURE, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "y").getModifyingProcNames();
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count("Example"));

        auto snd_result = get_var(kb_trivial, "flag").getModifyingProcNames();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("computeCentroid"));
        CHECK(snd_result.count("main"));

        auto trd_result = get_var(kb_trivial, "x").getModifyingProcNames();
        CHECK(trd_result.size() == 3);
        CHECK(trd_result.count("computeCentroid"));
        CHECK(trd_result.count("main"));
        CHECK(trd_result.count("readPoint"));
    }
}
