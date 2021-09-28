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

TEST_CASE("Parent(a, b)")
{
    SECTION("testing statements directly inside while loops")
    {
        CHECK(get_stmt(kb, 4).isParentOf(5));
        CHECK(get_stmt(kb, 4).isParentOf(6));
        CHECK(get_stmt(kb, 4).isParentOf(10));

        CHECK(get_stmt(kb, 14).isParentOf(15));
        CHECK(get_stmt(kb, 14).isParentOf(17));
    }

    SECTION("testing statements not directly inside while loops")
    {
        CHECK_FALSE(get_stmt(kb, 4).isParentOf(3));
        CHECK_FALSE(get_stmt(kb, 4).isParentOf(4));
        CHECK_FALSE(get_stmt(kb, 4).isParentOf(7));
        CHECK_FALSE(get_stmt(kb, 4).isParentOf(8));
        CHECK_FALSE(get_stmt(kb, 4).isParentOf(12));

        CHECK_FALSE(get_stmt(kb, 14).isParentOf(18));
        CHECK_FALSE(get_stmt(kb, 14).isParentOf(21));
    }

    SECTION("testing statements directly inside if statements")
    {
        CHECK(get_stmt(kb, 6).isParentOf(7));
        CHECK(get_stmt(kb, 6).isParentOf(8));

        CHECK(get_stmt(kb, 13).isParentOf(14));
        CHECK(get_stmt(kb, 13).isParentOf(18));
        CHECK(get_stmt(kb, 13).isParentOf(20));

        CHECK(get_stmt(kb, 22).isParentOf(23));
        CHECK(get_stmt(kb, 22).isParentOf(24));
    }

    SECTION("testing statements not directly inside if statements")
    {
        CHECK_FALSE(get_stmt(kb, 6).isParentOf(6));
        CHECK_FALSE(get_stmt(kb, 6).isParentOf(9));

        CHECK_FALSE(get_stmt(kb, 13).isParentOf(16));
        CHECK_FALSE(get_stmt(kb, 13).isParentOf(21));
    }

    SECTION("testing invalid queries")
    {
        CHECK_THROWS_WITH(get_stmt(kb, -1).isParentOf(-1), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 0).isParentOf(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 0).isParentOf(6), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_FALSE(get_stmt(kb, 6).isParentOf(0));
    }
}

TEST_CASE("Parent*(a, b)")
{
    SECTION("testing direct parents")
    {
        CHECK(get_stmt(kb, 4).isAncestorOf(5));
        CHECK(get_stmt(kb, 4).isAncestorOf(6));
        CHECK(get_stmt(kb, 4).isAncestorOf(10));

        CHECK(get_stmt(kb, 14).isAncestorOf(15));
        CHECK(get_stmt(kb, 14).isAncestorOf(17));

        CHECK(get_stmt(kb, 6).isAncestorOf(7));
        CHECK(get_stmt(kb, 6).isAncestorOf(8));

        CHECK(get_stmt(kb, 13).isAncestorOf(14));
        CHECK(get_stmt(kb, 13).isAncestorOf(18));
        CHECK(get_stmt(kb, 13).isAncestorOf(20));

        CHECK(get_stmt(kb, 22).isAncestorOf(23));
        CHECK(get_stmt(kb, 22).isAncestorOf(24));
    }

    SECTION("testing indirect parents")
    {
        CHECK(get_stmt(kb, 4).isAncestorOf(7));
        CHECK(get_stmt(kb, 4).isAncestorOf(8));

        CHECK(get_stmt(kb, 13).isAncestorOf(16));
        CHECK(get_stmt(kb, 13).isAncestorOf(17));
    }

    SECTION("testing neither direct nor indirect parents")
    {
        CHECK_FALSE(get_stmt(kb, 1).isAncestorOf(2));
        CHECK_FALSE(get_stmt(kb, 4).isAncestorOf(12));
        CHECK_FALSE(get_stmt(kb, 5).isAncestorOf(7));
        CHECK_FALSE(get_stmt(kb, 5).isAncestorOf(8));
        CHECK_FALSE(get_stmt(kb, 14).isAncestorOf(20));
        CHECK_FALSE(get_stmt(kb, 14).isAncestorOf(22));
    }

    SECTION("testing invalid queries")
    {
        CHECK_THROWS_WITH(get_stmt(kb, -1).isAncestorOf(-1), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 0).isAncestorOf(0), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_THROWS_WITH(get_stmt(kb, 0).isAncestorOf(6), Catch::Matchers::Contains("StatementNum is out of range"));
        CHECK_FALSE(get_stmt(kb, 6).isAncestorOf(0));
    }
}
