#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "simple/ast.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

using namespace util;
using namespace simple::ast;
using namespace simple::parser;
using namespace pkb;

static void req(bool b)
{
    REQUIRE(b);
}

constexpr const auto sample_source = R"(
    procedure Example {
        x = 6;
        y = 18;
        z = 12;

        tree = x + y + z;
        tree = x + y + z - z * z;
        tree = y * (w + x) - (w - y * z) - (w - x) - z;

        subtree = x + y;
        subtree = y + z;
        subtree = x + y + z;
        subtree = z - z;
        subtree = z * z;
        subtree = x - y - z;
        subtree = w - y * z;
        subtree = (w - x) - z;
        subtree = y * (w + x);
    }
)";

Expr* get_rhs(Stmt* stmt)
{
    return ((AssignStmt*) stmt)->rhs;
}

TEST_CASE("pattern matching on trees")
{
    auto prog = parseProgram(sample_source).unwrap();
    auto kb = processProgram(prog).unwrap();
    auto stmts = kb->statements;

    std::vector<Expr*> trees;
    for(int i = 3; i < 6; i++)
        trees.push_back(get_rhs(stmts[i]));

    std::vector<Expr*> subtrees;
    for(int i = 6; i < 15; i++)
        subtrees.push_back(get_rhs(stmts[i]));

    SECTION("testing positive exact matching tests")
    {
        req(exact_match(trees[0], trees[0]));
        req(exact_match(trees[0], subtrees[2]));
        req(exact_match(subtrees[3], subtrees[3]));
    }

    SECTION("testing negative exact matching tests")
    {
        req(!exact_match(subtrees[0], trees[0]));
        req(!exact_match(subtrees[0], subtrees[3]));
        req(!exact_match(subtrees[3], subtrees[6]));
    }

    SECTION("testing positive partial matching tests")
    {
        req(partial_match(subtrees[0], trees[0]));
        req(partial_match(subtrees[2], trees[1]));
        req(partial_match(subtrees[4], trees[1]));
        req(partial_match(subtrees[6], trees[2]));
        req(partial_match(subtrees[8], trees[2]));
    }

    SECTION("testing negative partial matching tests")
    {
        req(!partial_match(subtrees[1], trees[0]));
        req(!partial_match(subtrees[1], trees[1]));
        req(!partial_match(subtrees[3], trees[0]));
        req(!partial_match(subtrees[3], trees[1]));
        req(!partial_match(subtrees[5], trees[1]));
        req(!partial_match(subtrees[5], trees[2]));
        req(!partial_match(subtrees[6], trees[0]));
        req(!partial_match(subtrees[6], trees[1]));
        req(!partial_match(subtrees[7], trees[1]));
        req(!partial_match(subtrees[7], trees[2]));
    }
}
