#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "simple/ast.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"

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
    auto kb = processProgram(parseProgram(sample_source));
    std::vector<Stmt*> stmts {};
    {
        for(const auto& s : kb->uses_modifies.statements)
            stmts.push_back(s->stmt);
    }

    std::vector<Expr*> trees;
    for(int i = 3; i < 6; i++)
        trees.push_back(get_rhs(stmts[i]));

    std::vector<Expr*> subtrees;
    for(int i = 6; i < 15; i++)
        subtrees.push_back(get_rhs(stmts[i]));

    SECTION("testing positive exact matching tests")
    {
        req(exactMatch(trees[0], trees[0]));
        req(exactMatch(trees[0], subtrees[2]));
        req(exactMatch(subtrees[3], subtrees[3]));
    }

    SECTION("testing negative exact matching tests")
    {
        req(!exactMatch(subtrees[0], trees[0]));
        req(!exactMatch(subtrees[0], subtrees[3]));
        req(!exactMatch(subtrees[3], subtrees[6]));
    }

    SECTION("testing positive partial matching tests")
    {
        req(partialMatch(subtrees[0], trees[0]));
        req(partialMatch(subtrees[2], trees[1]));
        req(partialMatch(subtrees[4], trees[1]));
        req(partialMatch(subtrees[6], trees[2]));
        req(partialMatch(subtrees[8], trees[2]));
    }

    SECTION("testing negative partial matching tests")
    {
        req(!partialMatch(subtrees[1], trees[0]));
        req(!partialMatch(subtrees[1], trees[1]));
        req(!partialMatch(subtrees[3], trees[0]));
        req(!partialMatch(subtrees[3], trees[1]));
        req(!partialMatch(subtrees[5], trees[1]));
        req(!partialMatch(subtrees[5], trees[2]));
        req(!partialMatch(subtrees[6], trees[0]));
        req(!partialMatch(subtrees[6], trees[1]));
        req(!partialMatch(subtrees[7], trees[1]));
        req(!partialMatch(subtrees[7], trees[2]));
    }
}
