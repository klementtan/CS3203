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
    return *pkb->getStatementAtIndex(num);
}

TEST_CASE("Follows(a, b)")
{
    SECTION("Follows(a,b) in top level statement list")
    {
        CHECK(get_stmt(kb, 1).followedBy(2));
        CHECK(get_stmt(kb, 4).followedBy(12));
        CHECK(get_stmt(kb, 13).followedBy(21));
        CHECK(get_stmt(kb, 18).followedBy(19));
    }

    SECTION("Follows(a,b) in if/while for 1 and 2 levels of nesting")
    {
        CHECK(get_stmt(kb, 5).followedBy(6));
        CHECK(get_stmt(kb, 6).followedBy(9));
        CHECK(get_stmt(kb, 9).followedBy(10));
        CHECK(get_stmt(kb, 10).followedBy(11));
        CHECK(get_stmt(kb, 15).followedBy(16));
        CHECK(get_stmt(kb, 14).followedBy(18));
    }

    SECTION("Follows(a,b) negative test cases for diff stmtList, diff procs")
    {
        CHECK_FALSE(get_stmt(kb, 4).followedBy(5));
        CHECK_FALSE(get_stmt(kb, 6).followedBy(7));
        CHECK_FALSE(get_stmt(kb, 7).followedBy(8));
        CHECK_FALSE(get_stmt(kb, 8).followedBy(9));
        CHECK_FALSE(get_stmt(kb, 11).followedBy(12));
        CHECK_FALSE(get_stmt(kb, 10).followedBy(22));
        CHECK_FALSE(get_stmt(kb, 23).followedBy(24));
    }

    SECTION("Follows(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(get_stmt(kb, 0).followedBy(20), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, -1).followedBy(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 25).followedBy(26), Catch::Matchers::Contains("StatementNum is out of range"));
    }
}

TEST_CASE("Follows*(a, b)")
{
    SECTION("Follows*(a,b) in top level statement list such that Follows(a, b) holds")
    {
        CHECK(get_stmt(kb, 1).followedTransitivelyBy(2));
        CHECK(get_stmt(kb, 4).followedTransitivelyBy(12));
        CHECK(get_stmt(kb, 13).followedTransitivelyBy(21));
        CHECK(get_stmt(kb, 18).followedTransitivelyBy(19));
    }

    SECTION("Follows*(a,b) in top level statement list")
    {
        CHECK(get_stmt(kb, 1).followedTransitivelyBy(3));
        CHECK(get_stmt(kb, 1).followedTransitivelyBy(4));
        CHECK(get_stmt(kb, 1).followedTransitivelyBy(12));
        CHECK(get_stmt(kb, 2).followedTransitivelyBy(12));
    }

    SECTION("Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        CHECK(get_stmt(kb, 5).followedTransitivelyBy(6));
        CHECK(get_stmt(kb, 5).followedTransitivelyBy(9));
        CHECK(get_stmt(kb, 5).followedTransitivelyBy(10));
        CHECK(get_stmt(kb, 5).followedTransitivelyBy(11));
        CHECK(get_stmt(kb, 6).followedTransitivelyBy(11));
        CHECK(get_stmt(kb, 14).followedTransitivelyBy(18));
        CHECK(get_stmt(kb, 14).followedTransitivelyBy(19));
        CHECK(get_stmt(kb, 15).followedTransitivelyBy(17));
    }

    SECTION("Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        CHECK_FALSE(get_stmt(kb, 4).followedTransitivelyBy(6));
        CHECK_FALSE(get_stmt(kb, 6).followedTransitivelyBy(8));
        CHECK_FALSE(get_stmt(kb, 7).followedTransitivelyBy(8));
        CHECK_FALSE(get_stmt(kb, 8).followedTransitivelyBy(11));
        CHECK_FALSE(get_stmt(kb, 12).followedTransitivelyBy(13));
        CHECK_FALSE(get_stmt(kb, 10).followedTransitivelyBy(24));
        CHECK_FALSE(get_stmt(kb, 22).followedTransitivelyBy(24));
    }
    SECTION("Follows*(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(get_stmt(kb, 0).followedTransitivelyBy(20), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, -1).followedTransitivelyBy(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 25).followedTransitivelyBy(26), Catch::Matchers::Contains("StatementNum is out of range"));
    }
}

// Will comprise of all of the above since it will be replacing them
TEST_CASE("getFollows")
{
    SECTION("Follows(a,b) and Follows*(a,b) in top level statement list")
    {
        auto& fst_result = get_stmt(kb, 1);
        CHECK(fst_result.getDirectFollowee() == 0);
        CHECK(fst_result.getDirectFollower() == 2);
        CHECK(fst_result.getTransitiveFollowees().size() == 0);
        CHECK(fst_result.getTransitiveFollowers().size() == 4);
        CHECK(fst_result.getTransitiveFollowers().count(2) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(3) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(4) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(12) > 0);

        auto& snd_result = get_stmt(kb, 3);
        CHECK(snd_result.getDirectFollowee() == 2);
        CHECK(snd_result.getDirectFollower() == 4);
        CHECK(snd_result.getTransitiveFollowees().size() == 2);
        CHECK(snd_result.getTransitiveFollowers().size() == 2);
        CHECK(snd_result.getTransitiveFollowees().count(1) > 0);
        CHECK(snd_result.getTransitiveFollowees().count(2) > 0);
        CHECK(snd_result.getTransitiveFollowers().count(4) > 0);
        CHECK(snd_result.getTransitiveFollowers().count(12) > 0);
    }

    SECTION("Follows(a,b) and Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        auto& fst_result = get_stmt(kb, 5);
        CHECK(fst_result.getDirectFollowee() == 0);
        CHECK(fst_result.getDirectFollower() == 6);
        CHECK(fst_result.getTransitiveFollowees().size() == 0);
        CHECK(fst_result.getTransitiveFollowers().size() == 4);
        CHECK(fst_result.getTransitiveFollowers().count(6) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(9) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(10) > 0);
        CHECK(fst_result.getTransitiveFollowers().count(11) > 0);

        auto& snd_result = get_stmt(kb, 17);
        CHECK(snd_result.getDirectFollowee() == 16);
        CHECK(snd_result.getDirectFollower() == 0);
        CHECK(snd_result.getTransitiveFollowees().size() == 2);
        CHECK(snd_result.getTransitiveFollowers().size() == 0);
        CHECK(snd_result.getTransitiveFollowees().count(15) > 0);
        CHECK(snd_result.getTransitiveFollowees().count(16) > 0);
    }

    SECTION("Follows(a,b) and Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        auto& fst_result = get_stmt(kb, 23);
        CHECK(fst_result.getDirectFollowee() != 22);
        CHECK(fst_result.getDirectFollower() != 24);

        auto& snd_result = get_stmt(kb, 16);
        CHECK(snd_result.getDirectFollowee() == 15);
        CHECK(snd_result.getDirectFollower() != 22);
    }
}
