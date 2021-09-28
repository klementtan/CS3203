#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "design_extractor.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

using namespace simple::parser;
using namespace pkb;

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
        y = z + x; }
    z = z + x + i;
    call q;
    i = i - 1; }
    call p; }

    procedure p {
        if (x<0) then {
        while (i>0) {
            x = z * 3 + 2 * y;
            call q;
            i = i - 1; }
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

static auto kb = DesignExtractor(parseProgram(sample_source)).run();
static const Statement& get_stmt(const std::unique_ptr<pkb::ProgramKB>& pkb, simple::ast::StatementNum num)
{
    return *pkb->getStatementAt(num);
}

TEST_CASE("Follows(a, b)")
{
    SECTION("Follows(a,b) in top level statement list")
    {
        CHECK(get_stmt(kb, 1).isFollowedBy(2));
        CHECK(get_stmt(kb, 4).isFollowedBy(12));
        CHECK(get_stmt(kb, 13).isFollowedBy(21));
        CHECK(get_stmt(kb, 18).isFollowedBy(19));
    }

    SECTION("Follows(a,b) in if/while for 1 and 2 levels of nesting")
    {
        CHECK(get_stmt(kb, 5).isFollowedBy(6));
        CHECK(get_stmt(kb, 6).isFollowedBy(9));
        CHECK(get_stmt(kb, 9).isFollowedBy(10));
        CHECK(get_stmt(kb, 10).isFollowedBy(11));
        CHECK(get_stmt(kb, 15).isFollowedBy(16));
        CHECK(get_stmt(kb, 14).isFollowedBy(18));
    }

    SECTION("Follows(a,b) negative test cases for diff stmtList, diff procs")
    {
        CHECK_FALSE(get_stmt(kb, 4).isFollowedBy(5));
        CHECK_FALSE(get_stmt(kb, 6).isFollowedBy(7));
        CHECK_FALSE(get_stmt(kb, 7).isFollowedBy(8));
        CHECK_FALSE(get_stmt(kb, 8).isFollowedBy(9));
        CHECK_FALSE(get_stmt(kb, 11).isFollowedBy(12));
        CHECK_FALSE(get_stmt(kb, 10).isFollowedBy(22));
        CHECK_FALSE(get_stmt(kb, 23).isFollowedBy(24));
    }

    SECTION("Follows(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(get_stmt(kb, 0).isFollowedBy(20), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, -1).isFollowedBy(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 25).isFollowedBy(26), Catch::Matchers::Contains("StatementNum is out of range"));
    }
}

TEST_CASE("Follows*(a, b)")
{
    SECTION("Follows*(a,b) in top level statement list such that Follows(a, b) holds")
    {
        CHECK(get_stmt(kb, 1).isFollowedTransitivelyBy(2));
        CHECK(get_stmt(kb, 4).isFollowedTransitivelyBy(12));
        CHECK(get_stmt(kb, 13).isFollowedTransitivelyBy(21));
        CHECK(get_stmt(kb, 18).isFollowedTransitivelyBy(19));
    }

    SECTION("Follows*(a,b) in top level statement list")
    {
        CHECK(get_stmt(kb, 1).isFollowedTransitivelyBy(3));
        CHECK(get_stmt(kb, 1).isFollowedTransitivelyBy(4));
        CHECK(get_stmt(kb, 1).isFollowedTransitivelyBy(12));
        CHECK(get_stmt(kb, 2).isFollowedTransitivelyBy(12));
    }

    SECTION("Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        CHECK(get_stmt(kb, 5).isFollowedTransitivelyBy(6));
        CHECK(get_stmt(kb, 5).isFollowedTransitivelyBy(9));
        CHECK(get_stmt(kb, 5).isFollowedTransitivelyBy(10));
        CHECK(get_stmt(kb, 5).isFollowedTransitivelyBy(11));
        CHECK(get_stmt(kb, 6).isFollowedTransitivelyBy(11));
        CHECK(get_stmt(kb, 14).isFollowedTransitivelyBy(18));
        CHECK(get_stmt(kb, 14).isFollowedTransitivelyBy(19));
        CHECK(get_stmt(kb, 15).isFollowedTransitivelyBy(17));
    }

    SECTION("Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        CHECK_FALSE(get_stmt(kb, 4).isFollowedTransitivelyBy(6));
        CHECK_FALSE(get_stmt(kb, 6).isFollowedTransitivelyBy(8));
        CHECK_FALSE(get_stmt(kb, 7).isFollowedTransitivelyBy(8));
        CHECK_FALSE(get_stmt(kb, 8).isFollowedTransitivelyBy(11));
        CHECK_FALSE(get_stmt(kb, 12).isFollowedTransitivelyBy(13));
        CHECK_FALSE(get_stmt(kb, 10).isFollowedTransitivelyBy(24));
        CHECK_FALSE(get_stmt(kb, 22).isFollowedTransitivelyBy(24));
    }
    SECTION("Follows*(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(
            get_stmt(kb, 0).isFollowedTransitivelyBy(20), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(
            get_stmt(kb, -1).isFollowedTransitivelyBy(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(
            get_stmt(kb, 25).isFollowedTransitivelyBy(26), Catch::Matchers::Contains("StatementNum is out of range"));
    }
}

// Will comprise of all of the above since it will be replacing them
TEST_CASE("getFollows")
{
    SECTION("Follows(a,b) and Follows*(a,b) in top level statement list")
    {
        auto& fst_result = get_stmt(kb, 1);
        CHECK(fst_result.getStmtDirectlyBefore() == 0);
        CHECK(fst_result.getStmtDirectlyAfter() == 2);
        CHECK(fst_result.getStmtsTransitivelyBefore().size() == 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().size() == 4);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(2) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(3) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(4) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(12) > 0);

        auto& snd_result = get_stmt(kb, 3);
        CHECK(snd_result.getStmtDirectlyBefore() == 2);
        CHECK(snd_result.getStmtDirectlyAfter() == 4);
        CHECK(snd_result.getStmtsTransitivelyBefore().size() == 2);
        CHECK(snd_result.getStmtsTransitivelyAfter().size() == 2);
        CHECK(snd_result.getStmtsTransitivelyBefore().count(1) > 0);
        CHECK(snd_result.getStmtsTransitivelyBefore().count(2) > 0);
        CHECK(snd_result.getStmtsTransitivelyAfter().count(4) > 0);
        CHECK(snd_result.getStmtsTransitivelyAfter().count(12) > 0);
    }

    SECTION("Follows(a,b) and Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        auto& fst_result = get_stmt(kb, 5);
        CHECK(fst_result.getStmtDirectlyBefore() == 0);
        CHECK(fst_result.getStmtDirectlyAfter() == 6);
        CHECK(fst_result.getStmtsTransitivelyBefore().size() == 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().size() == 4);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(6) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(9) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(10) > 0);
        CHECK(fst_result.getStmtsTransitivelyAfter().count(11) > 0);

        auto& snd_result = get_stmt(kb, 17);
        CHECK(snd_result.getStmtDirectlyBefore() == 16);
        CHECK(snd_result.getStmtDirectlyAfter() == 0);
        CHECK(snd_result.getStmtsTransitivelyBefore().size() == 2);
        CHECK(snd_result.getStmtsTransitivelyAfter().size() == 0);
        CHECK(snd_result.getStmtsTransitivelyBefore().count(15) > 0);
        CHECK(snd_result.getStmtsTransitivelyBefore().count(16) > 0);
    }

    SECTION("Follows(a,b) and Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        auto& fst_result = get_stmt(kb, 23);
        CHECK(fst_result.getStmtDirectlyBefore() != 22);
        CHECK(fst_result.getStmtDirectlyAfter() != 24);

        auto& snd_result = get_stmt(kb, 16);
        CHECK(snd_result.getStmtDirectlyBefore() == 15);
        CHECK(snd_result.getStmtDirectlyAfter() != 22);
    }
}
