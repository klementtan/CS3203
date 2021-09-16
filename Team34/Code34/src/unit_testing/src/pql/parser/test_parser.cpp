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
    REQUIRE(query->declarations->declarations.size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations->declarations.find("v")->second;
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select->ent == v_declaration);
    REQUIRE(query->select->such_that == nullptr);
    REQUIRE(query->select->pattern == nullptr);
}

TEST_CASE("Follows* Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("stmt s;\n"
                                                   "Select s such that Follows* (6, s)");
    REQUIRE(query->declarations->declarations.size() == 1);
    pql::ast::Declaration* s_declaration = query->declarations->declarations.find("s")->second;
    REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
    REQUIRE(s_declaration->name == "s");
    REQUIRE(query->select->ent == s_declaration);
    pql::ast::SuchThatCl* such_that_cl = query->select->such_that;
    REQUIRE(such_that_cl->rel_conds.size() == 1);
    pql::ast::FollowsT* follows_t = dynamic_cast<pql::ast::FollowsT*>(*query->select->such_that->rel_conds.rbegin());
    REQUIRE(follows_t != nullptr);
    REQUIRE(dynamic_cast<pql::ast::StmtId*>(follows_t->before)->id == 6);
    REQUIRE(dynamic_cast<pql::ast::DeclaredStmt*>(follows_t->after)->declaration == s_declaration);
}

TEST_CASE("ModifiesS Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                   "Select v such that Modifies (6, v)");
    REQUIRE(query->declarations->declarations.size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations->declarations.find("v")->second;
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select->ent == v_declaration);
    pql::ast::SuchThatCl* such_that_cl = query->select->such_that;
    REQUIRE(such_that_cl->rel_conds.size() == 1);
    pql::ast::ModifiesS* modifies_s = dynamic_cast<pql::ast::ModifiesS*>(*query->select->such_that->rel_conds.rbegin());
    REQUIRE(modifies_s != nullptr);
    REQUIRE(dynamic_cast<pql::ast::StmtId*>(modifies_s->modifier)->id == 6);
    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(modifies_s->ent)->declaration == v_declaration);
}

TEST_CASE("ModifiesP Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v; procedure p;\n"
                                                   "Select p such that Modifies (p, \"x\")");
    REQUIRE(query->declarations->declarations.size() == 2);
    pql::ast::Declaration* p_declaration = query->declarations->declarations.find("p")->second;
    REQUIRE(p_declaration->design_ent == pql::ast::DESIGN_ENT::PROCEDURE);
    REQUIRE(p_declaration->name == "p");
    REQUIRE(query->select->ent == p_declaration);
    pql::ast::SuchThatCl* such_that_cl = query->select->such_that;
    REQUIRE(such_that_cl->rel_conds.size() == 1);
    pql::ast::ModifiesP* modifies_p = dynamic_cast<pql::ast::ModifiesP*>(*query->select->such_that->rel_conds.rbegin());
    REQUIRE(modifies_p != nullptr);
    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(modifies_p->modifier)->declaration == p_declaration);
    REQUIRE(dynamic_cast<pql::ast::EntName*>(modifies_p->ent)->name == "x");
}

TEST_CASE("UsesS Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                   "Select v such that Uses (14, v)");
    REQUIRE(query->declarations->declarations.size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations->declarations.find("v")->second;
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select->ent == v_declaration);
    pql::ast::SuchThatCl* such_that_cl = query->select->such_that;
    REQUIRE(such_that_cl->rel_conds.size() == 1);
    pql::ast::UsesS* uses_s = dynamic_cast<pql::ast::UsesS*>(*query->select->such_that->rel_conds.rbegin());
    REQUIRE(uses_s != nullptr);
    REQUIRE(dynamic_cast<pql::ast::StmtId*>(uses_s->user)->id == 14);
    REQUIRE(dynamic_cast<pql::ast::DeclaredEnt*>(uses_s->ent)->declaration == v_declaration);
}

TEST_CASE("Pattern Query")
{
    pql::ast::Query* query = pql::parser::parsePQL("assign a;\n"
                                                   "Select a pattern a ( \"normSq\" , _\"cenX * cenX\"_)");
    REQUIRE(query->declarations->declarations.size() == 1);
    pql::ast::Declaration* a_declaration = query->declarations->declarations.find("a")->second;
    REQUIRE(a_declaration->design_ent == pql::ast::DESIGN_ENT::ASSIGN);
    REQUIRE(a_declaration->name == "a");
    REQUIRE(query->select->ent == a_declaration);
    pql::ast::PatternCl* pattern_cl = query->select->pattern;
    REQUIRE(pattern_cl->pattern_conds.size() == 1);
    pql::ast::AssignPatternCond* assign_pattern_cond =
        dynamic_cast<pql::ast::AssignPatternCond*>(pattern_cl->pattern_conds.front());
    REQUIRE(assign_pattern_cond != nullptr);
    REQUIRE(assign_pattern_cond->assignment_declaration == a_declaration);
    REQUIRE(dynamic_cast<pql::ast::EntName*>(assign_pattern_cond->ent)->name == "normSq");
    pql::ast::ExprSpec* expr_spec = assign_pattern_cond->expr_spec;
    REQUIRE(expr_spec->is_subexpr == true);
    auto* expr = dynamic_cast<simple::ast::BinaryOp*>(expr_spec->expr);
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
        REQUIRE(query->declarations->declarations.size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations->declarations.find("s")->second;
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select->ent == s_declaration);
        pql::ast::SuchThatCl* such_that_cl = query->select->such_that;
        REQUIRE(such_that_cl->rel_conds.size() == 1);
        pql::ast::Parent* parent = dynamic_cast<pql::ast::Parent*>(*query->select->such_that->rel_conds.rbegin());
        REQUIRE(parent != nullptr);
        REQUIRE(dynamic_cast<pql::ast::StmtId*>(parent->parent)->id == 6);
        REQUIRE(dynamic_cast<pql::ast::DeclaredStmt*>(parent->child)->declaration == s_declaration);
    }
    SECTION("Invalid query")
    {
        REQUIRE_THROWS_WITH(pql::parser::parsePQL("stmt s;\nSelect s such that Parentt(6, s)"),
            Catch::Contains("Invalid relationship condition tokens"));
    }
}

TEST_CASE("invalid queries")
{
    SECTION("duplicate queries")
    {
        auto query = "stmt s; assign s; Select s";
        REQUIRE_THROWS_WITH(pql::parser::parsePQL(query), Catch::Contains("duplicate declaration 's'"));
    }

    SECTION("such-that spacing")
    {
        auto query = "stmt s; Select s such  that Parent(s, _)";
        REQUIRE_THROWS_WITH(pql::parser::parsePQL(query), Catch::Contains("Such That clause should start with 'such that'"));
    }
}
