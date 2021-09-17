// test_parser.cpp
//
// Unit test for pql/parser.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/parser.h"


TEST_CASE("Basic Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                   "Select v");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    auto v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.ent == v_declaration);
    REQUIRE(!query->select.such_that.has_value());
    REQUIRE(!query->select.pattern.has_value());
}

TEST_CASE("Follows* Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("stmt s;\n"
                                                   "Select s such that Follows* (6, s)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
    REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
    REQUIRE(s_declaration->name == "s");
    REQUIRE(query->select.ent == s_declaration);
    pql::ast::SuchThatCl& such_that_cl = *query->select.such_that;
    REQUIRE(such_that_cl.rel_conds.size() == 1);
    auto follows_t = dynamic_cast<pql::ast::FollowsT*>(such_that_cl.rel_conds.begin()->get());
    REQUIRE(follows_t != nullptr);

    CHECK(follows_t->before.isStatementId());
    CHECK(follows_t->before.id == 6);
    CHECK(follows_t->after.isDeclaration());
    CHECK(follows_t->after.declaration == s_declaration);
}

TEST_CASE("ModifiesS Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                   "Select v such that Modifies (6, v)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.ent == v_declaration);
    pql::ast::SuchThatCl& such_that_cl = *query->select.such_that;
    REQUIRE(such_that_cl.rel_conds.size() == 1);
    auto modifies_s = dynamic_cast<pql::ast::ModifiesS*>(such_that_cl.rel_conds.begin()->get());
    REQUIRE(modifies_s != nullptr);

    CHECK(modifies_s->modifier.isStatementId());
    CHECK(modifies_s->modifier.id == 6);

    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(modifies_s->ent)->declaration == v_declaration);
}

TEST_CASE("ModifiesP Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v; procedure p;\n"
                                                   "Select p such that Modifies (p, \"x\")");
    REQUIRE(query->declarations.getAllDeclarations().size() == 2);
    pql::ast::Declaration* p_declaration = query->declarations.getDeclaration("p");
    REQUIRE(p_declaration->design_ent == pql::ast::DESIGN_ENT::PROCEDURE);
    REQUIRE(p_declaration->name == "p");
    REQUIRE(query->select.ent == p_declaration);
    pql::ast::SuchThatCl& such_that_cl = *query->select.such_that;
    REQUIRE(such_that_cl.rel_conds.size() == 1);
    auto modifies_p = dynamic_cast<pql::ast::ModifiesP*>(such_that_cl.rel_conds.begin()->get());
    REQUIRE(modifies_p != nullptr);

    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(modifies_p->modifier)->declaration == p_declaration);
    REQUIRE(dynamic_cast<pql::ast::EntName*>(modifies_p->ent)->name == "x");
}

TEST_CASE("UsesS Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                   "Select v such that Uses (14, v)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.ent == v_declaration);
    pql::ast::SuchThatCl& such_that_cl = *query->select.such_that;
    REQUIRE(such_that_cl.rel_conds.size() == 1);
    auto uses_s = dynamic_cast<pql::ast::UsesS*>(such_that_cl.rel_conds.begin()->get());
    REQUIRE(uses_s != nullptr);

    CHECK(uses_s->user.isStatementId());
    CHECK(uses_s->user.id == 14);

    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(uses_s->ent)->declaration == v_declaration);
}

TEST_CASE("Pattern Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("assign a;\n"
                                                   "Select a pattern a ( \"normSq\" , _\"cenX * cenX\"_)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* a_declaration = query->declarations.getDeclaration("a");
    REQUIRE(a_declaration->design_ent == pql::ast::DESIGN_ENT::ASSIGN);
    REQUIRE(a_declaration->name == "a");
    REQUIRE(query->select.ent == a_declaration);
    pql::ast::PatternCl& pattern_cl = *query->select.pattern;
    REQUIRE(pattern_cl.pattern_conds.size() == 1);
    pql::ast::AssignPatternCond* assign_pattern_cond =
        dynamic_cast<pql::ast::AssignPatternCond*>(pattern_cl.pattern_conds.front().get());
    REQUIRE(assign_pattern_cond != nullptr);
    REQUIRE(assign_pattern_cond->assignment_declaration == a_declaration);
    REQUIRE(dynamic_cast<pql::ast::EntName*>(assign_pattern_cond->ent)->name == "normSq");
    auto& expr_spec = assign_pattern_cond->expr_spec;
    REQUIRE(expr_spec.is_subexpr == true);
    auto* expr = dynamic_cast<simple::ast::BinaryOp*>(expr_spec.expr.get());
    REQUIRE(expr);
    REQUIRE(expr->op == "*");
    auto* lhs = dynamic_cast<simple::ast::VarRef*>(expr->lhs);
    REQUIRE(lhs);
    REQUIRE(lhs->name == "cenX");
    auto* rhs = dynamic_cast<simple::ast::VarRef*>(expr->rhs);
    REQUIRE(rhs);
    REQUIRE(rhs->name == "cenX");
}

TEST_CASE("Parent Query")
{
    SECTION("Valid query")
    {
        pql::ast::Query* query = pql::parser::parsePQL("stmt s;\n"
                                                       "Select s such that Parent(6, s)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.ent == s_declaration);
        pql::ast::SuchThatCl& such_that_cl = *query->select.such_that;
        REQUIRE(such_that_cl.rel_conds.size() == 1);
        auto parent = dynamic_cast<pql::ast::Parent*>(such_that_cl.rel_conds.begin()->get());
        REQUIRE(parent != nullptr);

        CHECK(parent->parent.isStatementId());
        CHECK(parent->parent.id == 6);
        CHECK(parent->child.isDeclaration());
        CHECK(parent->child.declaration == s_declaration);
    }
    SECTION("Invalid query")
    {
        REQUIRE_THROWS_WITH(pql::parser::parsePQL("stmt s;\nSelect s such that Parentt(6, s)"),
            Catch::Contains("Invalid relationship condition tokens"));
    }
}

TEST_CASE("invalid queries")
{
    using namespace pql::parser;

    SECTION("duplicate queries")
    {
        auto query = "stmt s; assign s; Select s";
        REQUIRE_THROWS_WITH(parsePQL(query), Catch::Contains("duplicate declaration 's'"));
    }

    SECTION("such-that/parent*/follows* spacing")
    {
        CHECK_THROWS_WITH(parsePQL("stmt s ; Select s   such    that   Parent(s, _)"),
            Catch::Contains("Such That clause should start with 'such that'"));

        CHECK_THROWS_WITH(parsePQL("stmt s; Select s such that Follows  *  (s, _)"),
            Catch::Contains("FollowsT relationship condition should start with 'Follows*'"));

        CHECK_THROWS_WITH(parsePQL("stmt s; Select s such that Parent  *  (s, _)"),
            Catch::Contains("ParentT relationship condition should start with 'Parent*'"));
    }
}
