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
    return *pkb->getStatementAtIndex(num);
}

static const Procedure& get_proc(const std::unique_ptr<pkb::ProgramKB>& pkb, const char* name)
{
    return pkb->getProcedureNamed(name);
}

static const Variable& get_var(const std::unique_ptr<pkb::ProgramKB>& pkb, const char* name)
{
    return pkb->getVariableNamed(name);
}

TEST_CASE("Uses(DeclaredStmt, DeclaredVar)")
{
    SECTION("Uses(DeclaredStmt, DeclaredVar) for assignment, condition and print")
    {
        CHECK(get_stmt(kb_sample, 4).usesVariable("i"));
        CHECK(get_stmt(kb_sample, 6).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 9).usesVariable("i"));
        CHECK(get_stmt(kb_sample, 9).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 9).usesVariable("z"));
        CHECK(get_stmt(kb_sample, 13).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 14).usesVariable("i"));

        CHECK(get_stmt(kb_trivial, 6).usesVariable("flag"));
        CHECK(get_stmt(kb_trivial, 9).usesVariable("normSq"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("x"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("y"));
        CHECK(get_stmt(kb_trivial, 19).usesVariable("count"));
        CHECK(get_stmt(kb_trivial, 23).usesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 23).usesVariable("cenY"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) inside if/while")
    {
        CHECK(get_stmt(kb_sample, 4).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 4).usesVariable("z"));
        CHECK(get_stmt(kb_sample, 6).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 6).usesVariable("z"));
        CHECK(get_stmt(kb_sample, 13).usesVariable("i"));
        CHECK(get_stmt(kb_sample, 13).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 13).usesVariable("y"));
        CHECK(get_stmt(kb_sample, 13).usesVariable("z"));

        CHECK(get_stmt(kb_trivial, 14).usesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("count"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("x"));
        CHECK(get_stmt(kb_trivial, 14).usesVariable("y"));
        CHECK(get_stmt(kb_trivial, 19).usesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 19).usesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 19).usesVariable("count"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) for procCall")
    {
        CHECK(get_stmt(kb_sample, 10).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 10).usesVariable("z"));
        CHECK(get_stmt(kb_sample, 12).usesVariable("i"));
        CHECK(get_stmt(kb_sample, 12).usesVariable("x"));
        CHECK(get_stmt(kb_sample, 12).usesVariable("y"));
        CHECK(get_stmt(kb_sample, 12).usesVariable("z"));

        CHECK(get_stmt(kb_trivial, 2).usesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 2).usesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 2).usesVariable("count"));
        CHECK(get_stmt(kb_trivial, 2).usesVariable("x"));
        CHECK(get_stmt(kb_trivial, 2).usesVariable("y"));
        CHECK(get_stmt(kb_trivial, 3).usesVariable("cenX"));
        CHECK(get_stmt(kb_trivial, 3).usesVariable("cenY"));
        CHECK(get_stmt(kb_trivial, 3).usesVariable("flag"));
        CHECK(get_stmt(kb_trivial, 3).usesVariable("normSq"));
    }

    SECTION("Uses(DeclaredStmt, DeclaredVar) negative test cases")
    {
        CHECK_FALSE(get_stmt(kb_sample, 3).usesVariable("i"));
        CHECK_FALSE(get_stmt(kb_sample, 6).usesVariable("y"));
        CHECK_FALSE(get_stmt(kb_sample, 7).usesVariable("z"));
        CHECK_FALSE(get_stmt(kb_sample, 10).usesVariable("i"));


        CHECK_FALSE(get_stmt(kb_trivial, 1).usesVariable("flag"));
        CHECK_FALSE(get_stmt(kb_trivial, 14).usesVariable("flag"));
        CHECK_FALSE(get_stmt(kb_trivial, 18).usesVariable("x"));
        CHECK_FALSE(get_stmt(kb_trivial, 18).usesVariable("y"));
    }
}

TEST_CASE("Uses(DeclaredProc, DeclaredVar)")
{
    SECTION("Uses(DeclaredProc, DeclaredVar)")
    {
        CHECK(get_proc(kb_sample, "Example").usesVariable("i"));
        CHECK(get_proc(kb_sample, "Example").usesVariable("x"));
        CHECK(get_proc(kb_sample, "Example").usesVariable("y"));
        CHECK(get_proc(kb_sample, "Example").usesVariable("z"));
        CHECK(get_proc(kb_sample, "p").usesVariable("i"));
        CHECK(get_proc(kb_sample, "p").usesVariable("x"));
        CHECK(get_proc(kb_sample, "p").usesVariable("y"));
        CHECK(get_proc(kb_sample, "p").usesVariable("z"));
        CHECK(get_proc(kb_sample, "q").usesVariable("x"));
        CHECK(get_proc(kb_sample, "q").usesVariable("z"));

        CHECK(get_proc(kb_trivial, "main").usesVariable("cenX"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("cenY"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("count"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("flag"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("normSq"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("x"));
        CHECK(get_proc(kb_trivial, "main").usesVariable("y"));
        CHECK(get_proc(kb_trivial, "printResults").usesVariable("cenX"));
        CHECK(get_proc(kb_trivial, "printResults").usesVariable("cenY"));
        CHECK(get_proc(kb_trivial, "printResults").usesVariable("flag"));
        CHECK(get_proc(kb_trivial, "printResults").usesVariable("normSq"));
        CHECK(get_proc(kb_trivial, "computeCentroid").usesVariable("cenX"));
        CHECK(get_proc(kb_trivial, "computeCentroid").usesVariable("cenY"));
        CHECK(get_proc(kb_trivial, "computeCentroid").usesVariable("count"));
        CHECK(get_proc(kb_trivial, "computeCentroid").usesVariable("x"));
        CHECK(get_proc(kb_trivial, "computeCentroid").usesVariable("y"));
    }

    SECTION("Uses(DeclaredProc, DeclaredVar) negative test cases")
    {
        CHECK_FALSE(get_proc(kb_sample, "q").usesVariable("y"));
        CHECK_FALSE(get_proc(kb_sample, "q").usesVariable("i"));


        CHECK_FALSE(get_proc(kb_trivial, "printResults").usesVariable("x"));
        CHECK_FALSE(get_proc(kb_trivial, "printResults").usesVariable("y"));
        CHECK_FALSE(get_proc(kb_trivial, "computeCentroid").usesVariable("flag"));
        CHECK_FALSE(get_proc(kb_trivial, "computeCentroid").usesVariable("normSq"));
    }
}

TEST_CASE("Uses(DeclaredStmt, AllVar)")
{
    SECTION("Uses(DeclaredStmt, AllVar) for assignment and print")
    {
        auto& fst_result = get_stmt(kb_sample, 1).getUsedVariables();
        CHECK(fst_result.size() == 0);

        auto& snd_result = get_stmt(kb_sample, 5).getUsedVariables();
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count("x"));

        auto& trd_result = get_stmt(kb_trivial, 6).getUsedVariables();
        CHECK(trd_result.size() == 1);
        CHECK(trd_result.count("flag"));
    }

    SECTION("Uses(DeclaredStmt, AllVar) inside if/while")
    {
        auto& fst_result = get_stmt(kb_sample, 6).getUsedVariables();
        CHECK(fst_result.size() == 2);
        CHECK(fst_result.count("x"));
        CHECK(fst_result.count("z"));

        auto& snd_result = get_stmt(kb_sample, 14).getUsedVariables();
        CHECK(snd_result.size() == 4);
        CHECK(snd_result.count("i"));
        CHECK(snd_result.count("x"));
        CHECK(snd_result.count("y"));
        CHECK(snd_result.count("z"));
    }

    SECTION("Uses(DeclaredStmt, AllVar) for procCalls")
    {
        auto& fst_result = get_stmt(kb_trivial, 3).getUsedVariables();
        CHECK(fst_result.size() == 4);
        CHECK(fst_result.count("cenX"));
        CHECK(fst_result.count("cenY"));
        CHECK(fst_result.count("flag"));
        CHECK(fst_result.count("normSq"));

        auto& snd_result = get_stmt(kb_sample, 10).getUsedVariables();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("x"));
        CHECK(snd_result.count("z"));
    }
}

TEST_CASE("Uses(DeclaredProc, AllVar)")
{
    SECTION("Uses(DeclaredProc, AllVar) for assignment and print")
    {
        auto& fst_result = get_proc(kb_trivial, "main").getUsedVariables();
        CHECK(fst_result.size() == 7);
        CHECK(fst_result.count("cenX"));
        CHECK(fst_result.count("cenY"));
        CHECK(fst_result.count("count"));
        CHECK(fst_result.count("flag"));
        CHECK(fst_result.count("normSq"));
        CHECK(fst_result.count("x"));
        CHECK(fst_result.count("y"));

        auto& snd_result = get_proc(kb_trivial, "readPoint").getUsedVariables();
        CHECK(snd_result.size() == 0);

        auto& trd_result = get_proc(kb_trivial, "computeCentroid").getUsedVariables();
        CHECK(trd_result.size() == 5);
        CHECK(trd_result.count("cenX"));
        CHECK(trd_result.count("cenY"));
        CHECK(trd_result.count("count"));
        CHECK(trd_result.count("x"));
        CHECK(trd_result.count("y"));

        auto& frh_result = get_proc(kb_trivial, "printResults").getUsedVariables();
        CHECK(frh_result.size() == 4);
        CHECK(frh_result.count("cenX"));
        CHECK(frh_result.count("cenY"));
        CHECK(frh_result.count("flag"));
        CHECK(frh_result.count("normSq"));
    }
}

TEST_CASE("Uses(Synonym, DeclaredVar)")
{
    SECTION("Uses(ASSIGN, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "count").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::ASSIGN);
        CHECK(fst_result.size() == 3);
        CHECK(fst_result.count(15));
        CHECK(fst_result.count(21));
        CHECK(fst_result.count(22));

        auto snd_result = get_var(kb_trivial, "cenX").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::ASSIGN);
        CHECK(snd_result.size() == 3);
        CHECK(snd_result.count(16));
        CHECK(snd_result.count(21));
        CHECK(snd_result.count(23));
    }

    SECTION("Uses(PRINT, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "flag").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::PRINT);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(6));

        auto snd_result = get_var(kb_trivial, "cenX").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::PRINT);
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count(7));
    }

    SECTION("Uses(CALL, DeclaredVar)")
    {
        auto fst_result = get_var(kb_trivial, "flag").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::CALL);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(3));

        auto snd_result = get_var(kb_trivial, "cenX").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::CALL);
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count(2));
        CHECK(snd_result.count(3));
    }

    SECTION("Uses(IF, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "x").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::IF);
        CHECK(fst_result.size() == 3);
        CHECK(fst_result.count(6));
        CHECK(fst_result.count(13));
        CHECK(fst_result.count(22));

        auto snd_result = get_var(kb_sample, "i").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::IF);
        CHECK(snd_result.size() == 1);
        CHECK(snd_result.count(13));
    }

    SECTION("Uses(WHILE, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "y").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::WHILE);
        CHECK(fst_result.size() == 1);
        CHECK(fst_result.count(14));

        auto snd_result = get_var(kb_sample, "i").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::WHILE);
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count(4));
        CHECK(snd_result.count(14));
    }

    SECTION("Uses(STMT, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "x").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::STMT);
        CHECK(fst_result.size() == 16);
        CHECK(fst_result.count(4));
        CHECK(fst_result.count(5));
        CHECK(fst_result.count(6));
        CHECK(fst_result.count(7));
        CHECK(fst_result.count(9));
        CHECK(fst_result.count(10));
        CHECK(fst_result.count(12));
        CHECK(fst_result.count(13));
        CHECK(fst_result.count(14));
        CHECK(fst_result.count(16));
        CHECK(fst_result.count(18));
        CHECK(fst_result.count(19));
        CHECK(fst_result.count(21));
        CHECK(fst_result.count(22));
        CHECK(fst_result.count(23));
        CHECK(fst_result.count(24));

        auto snd_result = get_var(kb_trivial, "x").getUsingStmtNumsFiltered(pql::ast::DESIGN_ENT::STMT);
        CHECK(snd_result.size() == 3);
        CHECK(snd_result.count(2));
        CHECK(snd_result.count(14));
        CHECK(snd_result.count(16));
    }

    SECTION("Uses(PROCEDURE, DeclaredVar)")
    {
        auto fst_result = get_var(kb_sample, "y").getUsingProcNames();
        CHECK(fst_result.size() == 2);
        CHECK(fst_result.count("Example"));
        CHECK(fst_result.count("p"));

        auto snd_result = get_var(kb_trivial, "flag").getUsingProcNames();
        CHECK(snd_result.size() == 2);
        CHECK(snd_result.count("printResults"));
        CHECK(snd_result.count("main"));
    }
}
