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

auto prog = parseProgram(sample_source).unwrap();
auto kb = processProgram(prog).unwrap();

TEST_CASE("Parent(a, b)")
{
    SECTION("testing statements directly inside while loops")
    {
        req(kb->isParent(4, 5));
        req(kb->isParent(4, 6));
        req(kb->isParent(4, 10));

        req(kb->isParent(14, 15));
        req(kb->isParent(14, 17));
    }

    SECTION("testing statements not directly inside while loops")
    {
        req(!kb->isParent(4, 3));
        req(!kb->isParent(4, 4));
        req(!kb->isParent(4, 7));
        req(!kb->isParent(4, 8));
        req(!kb->isParent(4, 12));

        req(!kb->isParent(14, 18));
        req(!kb->isParent(14, 21));
    }

    SECTION("testing statements directly inside if statements")
    {
        req(kb->isParent(6, 7));
        req(kb->isParent(6, 8));

        req(kb->isParent(13, 14));
        req(kb->isParent(13, 18));
        req(kb->isParent(13, 20));

        req(kb->isParent(22, 23));
        req(kb->isParent(22, 24));
    }

    SECTION("testing statements not directly inside if statements")
    {
        req(!kb->isParent(6, 6));
        req(!kb->isParent(6, 9));

        req(!kb->isParent(13, 16));
        req(!kb->isParent(13, 21));
    }

    SECTION("testing invalid queries")
    {
        req(!kb->isParent(-1, -1));
        req(!kb->isParent(0, 0));
        req(!kb->isParent(0, 6));
        req(!kb->isParent(6, 0));
    }
}

TEST_CASE("Parent*(a, b)")
{
    SECTION("testing direct parents")
    {
        req(kb->isParentT(4, 5));
        req(kb->isParentT(4, 6));
        req(kb->isParentT(4, 10));

        req(kb->isParentT(14, 15));
        req(kb->isParentT(14, 17));

        req(kb->isParentT(6, 7));
        req(kb->isParentT(6, 8));

        req(kb->isParentT(13, 14));
        req(kb->isParentT(13, 18));
        req(kb->isParentT(13, 20));

        req(kb->isParentT(22, 23));
        req(kb->isParentT(22, 24));
    }

    SECTION("testing indirect parents")
    {
        req(kb->isParentT(4, 7));
        req(kb->isParentT(4, 8));

        req(kb->isParentT(13, 16));
        req(kb->isParentT(13, 17));
    }

    SECTION("testing neither direct nor indirect parents")
    {
        req(!kb->isParentT(1, 2));
        req(!kb->isParentT(4, 12));
        req(!kb->isParentT(5, 7));
        req(!kb->isParentT(5, 8));
        req(!kb->isParentT(14, 20));
        req(!kb->isParentT(14, 22));
    }

    SECTION("testing invalid queries")
    {
        req(!kb->isParentT(-1, -1));
        req(!kb->isParentT(0, 0));
        req(!kb->isParentT(0, 6));
        req(!kb->isParentT(6, 0));
    }
}
