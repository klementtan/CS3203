#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

using namespace simple::parser;
using namespace pkb;

static void req(bool b)
{
    REQUIRE(b);
}

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

auto kb_parents = processProgram(parseProgram(sample_source));

TEST_CASE("Follows(a, b)")
{
    SECTION("Follows(a,b) in top level statement list")
    {
        req(kb_parents->isFollows(1, 2));
        req(kb_parents->isFollows(4, 12));
        req(kb_parents->isFollows(13, 21));
        req(kb_parents->isFollows(18, 19));
    }

    SECTION("Follows(a,b) in if/while for 1 and 2 levels of nesting")
    {
        req(kb_parents->isFollows(5, 6));
        req(kb_parents->isFollows(6, 9));
        req(kb_parents->isFollows(9, 10));
        req(kb_parents->isFollows(10, 11));
        req(kb_parents->isFollows(15, 16));
        req(kb_parents->isFollows(14, 18));
    }

    SECTION("Follows(a,b) negative test cases for diff stmtList, diff procs")
    {
        req(!kb_parents->isFollows(4, 5));
        req(!kb_parents->isFollows(6, 7));
        req(!kb_parents->isFollows(7, 8));
        req(!kb_parents->isFollows(8, 9));
        req(!kb_parents->isFollows(11, 12));
        req(!kb_parents->isFollows(10, 22));
        req(!kb_parents->isFollows(23, 24));
    }

    SECTION("Follows(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(kb_parents->isFollows(0, 20), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->isFollows(-1, 0), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->isFollows(24, 25), Catch::Matchers::Contains("StatementNum out of range."));
    }
}

TEST_CASE("Follows*(a, b)")
{
    SECTION("Follows*(a,b) in top level statement list such that Follows(a, b) holds")
    {
        req(kb_parents->isFollowsT(1, 2));
        req(kb_parents->isFollowsT(4, 12));
        req(kb_parents->isFollowsT(13, 21));
        req(kb_parents->isFollowsT(18, 19));
    }

    SECTION("Follows*(a,b) in top level statement list")
    {
        req(kb_parents->isFollowsT(1, 3));
        req(kb_parents->isFollowsT(1, 4));
        req(kb_parents->isFollowsT(1, 12));
        req(kb_parents->isFollowsT(2, 12));
    }

    SECTION("Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        req(kb_parents->isFollowsT(5, 6));
        req(kb_parents->isFollowsT(5, 9));
        req(kb_parents->isFollowsT(5, 10));
        req(kb_parents->isFollowsT(5, 11));
        req(kb_parents->isFollowsT(6, 11));
        req(kb_parents->isFollowsT(14, 18));
        req(kb_parents->isFollowsT(14, 19));
        req(kb_parents->isFollowsT(15, 17));
    }

    SECTION("Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        req(!kb_parents->isFollowsT(4, 6));
        req(!kb_parents->isFollowsT(6, 8));
        req(!kb_parents->isFollowsT(7, 8));
        req(!kb_parents->isFollowsT(8, 11));
        req(!kb_parents->isFollowsT(12, 13));
        req(!kb_parents->isFollowsT(10, 24));
        req(!kb_parents->isFollowsT(22, 24));
    }
    SECTION("Follows*(a,b) fail from indices out of range")
    {
        CHECK_THROWS_WITH(kb_parents->isFollowsT(0, 20), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->isFollowsT(-1, 0), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->isFollowsT(24, 25), Catch::Matchers::Contains("StatementNum out of range."));
    }
}

// Will comprise of all of the above since it will be replacing them
TEST_CASE("getFollows")
{
    SECTION("Follows(a,b) and Follows*(a,b) in top level statement list")
    {
        auto fst_result = kb_parents->getFollows(1);
        req(fst_result->directly_before == 0);
        req(fst_result->directly_after == 2);
        req(fst_result->before.size() == 0);
        req(fst_result->after.size() == 4);
        req(fst_result->after.count(2));
        req(fst_result->after.count(3));
        req(fst_result->after.count(4));
        req(fst_result->after.count(12));

        auto snd_result = kb_parents->getFollows(3);
        req(snd_result->directly_before == 2);
        req(snd_result->directly_after == 4);
        req(snd_result->before.size() == 2);
        req(snd_result->after.size() == 2);
        req(snd_result->before.count(1));
        req(snd_result->before.count(2));
        req(snd_result->after.count(4));
        req(snd_result->after.count(12));
    }

    SECTION("Follows(a,b) and Follows*(a,b) in if/while for 1 and 2 levels of nesting")
    {
        auto fst_result = kb_parents->getFollows(5);
        req(fst_result->directly_before == 0);
        req(fst_result->directly_after == 6);
        req(fst_result->before.size() == 0);
        req(fst_result->after.size() == 4);
        req(fst_result->after.count(6));
        req(fst_result->after.count(9));
        req(fst_result->after.count(10));
        req(fst_result->after.count(11));

        auto snd_result = kb_parents->getFollows(17);
        req(snd_result->directly_before == 16);
        req(snd_result->directly_after == 0);
        req(snd_result->before.size() == 2);
        req(snd_result->after.size() == 0);
        req(snd_result->before.count(15));
        req(snd_result->before.count(16));
    }

    SECTION("Follows(a,b) and Follows*(a,b) negative test cases for diff stmtList, diff procs")
    {
        auto fst_result = kb_parents->getFollows(23);
        req(fst_result->directly_before != 22);
        req(fst_result->directly_after != 24);

        auto snd_result = kb_parents->getFollows(16);
        req(snd_result->directly_before == 15);
        req(snd_result->directly_after != 22);
    }

    SECTION("getFollows(stmt_no) fail from indices out of range")
    {
        CHECK_THROWS_WITH(kb_parents->getFollows(0), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->getFollows(-1), Catch::Matchers::Contains("StatementNum out of range."));
        CHECK_THROWS_WITH(kb_parents->getFollows(25), Catch::Matchers::Contains("StatementNum out of range."));
    }
}
