// test_ast.cpp
//
// Unit test for pql/ast.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/ast.h"
#include "simple/ast.h"
#include <iostream>
#include <unordered_map>
#include "util.h"

using namespace pql::ast;

TEST_CASE("Declaration")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    REQUIRE(declaration->toString() == "Declaration(ent:assign, name:foo)");
}

TEST_CASE("DeclarationList")
{
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::PROCEDURE };

    auto declaration_list = pql::ast::DeclarationList {};
    declaration_list.addDeclaration("foo", declaration1);
    declaration_list.addDeclaration("bar", declaration2);

    REQUIRE(declaration_list.toString() == "DeclarationList[\n"
                                           "\tname:bar, declaration:Declaration(ent:procedure, name:bar)\n"
                                           "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
                                           "]");
}

TEST_CASE("DeclaredStmt")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration);
    REQUIRE(stmt.toString() == "DeclaredStmt(declaration: Declaration(ent:assign, name:foo))");
}

TEST_CASE("StmtId")
{
    auto stmt = StmtRef::ofStatementId(1);
    REQUIRE(stmt.toString() == "StmtId(id:1)");
}

TEST_CASE("AllStmt")
{
    auto stmt = StmtRef::ofWildcard();
    REQUIRE(stmt.toString() == "AllStmt(name: _)");
}

TEST_CASE("DeclaredEnt")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration;
    pql::ast::EntRef* ent = &declared_ent;
    REQUIRE(ent->toString() == "DeclaredEnt(declaration:Declaration(ent:assign, name:foo))");
}

TEST_CASE("EntName")
{
    pql::ast::EntName ent_name;
    ent_name.name = "foo";
    pql::ast::EntRef* ent = &ent_name;
    REQUIRE(ent->toString() == "EntName(name:foo)");
}

TEST_CASE("AllEnt")
{
    pql::ast::AllEnt all_ent;
    pql::ast::EntRef* ent = &all_ent;
    INFO(ent->toString());
    REQUIRE(ent->toString() == "AllEnt(name: _)");
}

TEST_CASE("ModifiesS")
{
    /** Initialize entity declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration1;
    pql::ast::EntRef* ent = &declared_ent;

    /** Initialize stmt declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::ModifiesS relationship;
    relationship.ent = ent;
    relationship.modifier = stmt;
    INFO(relationship.toString())
    REQUIRE(relationship.toString() ==
            "ModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)),"
            " ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))");
}

TEST_CASE("UsesS")
{
    /** Initialize entity declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration1;
    pql::ast::EntRef* ent = &declared_ent;

    /** Initialize stmt declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::UsesS relationship;
    relationship.ent = ent;
    relationship.user = stmt;
    INFO(relationship.toString());
    REQUIRE(relationship.toString() == "UsesS(user: DeclaredStmt(declaration: Declaration(ent:assign, name:bar)), "
                                       "ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))");
}

TEST_CASE("Parent")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(declaration1);

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::Parent relationship;
    relationship.parent = stmt1;
    relationship.child = stmt2;
    INFO(relationship.toString());
    REQUIRE(relationship.toString() == "Parent(parent:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)),"
                                       " child:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("ParentT")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(declaration1);

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::ParentT relationship;
    relationship.ancestor = stmt1;
    relationship.descendant = stmt2;
    INFO(relationship.toString());
    REQUIRE(relationship.toString() == "ParentT(ancestor:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)), "
                                       "descendant:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("Follows")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(declaration1);

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::Follows relationship;
    relationship.directly_before = stmt1;
    relationship.directly_after = stmt2;
    INFO(relationship.toString());
    REQUIRE(relationship.toString() == "Follows(directly_before:DeclaredStmt(declaration: Declaration(ent:assign, "
                                       "name:foo)), directly_after:DeclaredStmt(declaration: Declaration(ent:assign, "
                                       "name:bar)))");
}

TEST_CASE("FollowsT")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(declaration1);

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    pql::ast::FollowsT relationship;
    relationship.before = stmt1;
    relationship.after = stmt2;
    INFO(relationship.toString());
    REQUIRE(relationship.toString() == "FollowsT(before:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)), "
                                       "afterDeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("ExprSpec")
{
    simple::ast::BinaryOp* expr = new simple::ast::BinaryOp {};
    simple::ast::VarRef* lhs = new simple::ast::VarRef {};
    lhs->name = "x";
    simple::ast::VarRef* rhs = new simple::ast::VarRef {};
    rhs->name = "y";
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = "+";
    pql::ast::ExprSpec* expr_spec = new pql::ast::ExprSpec { true, std::unique_ptr<simple::ast::Expr>(expr) };

    REQUIRE(expr_spec->toString() == "ExprSpec(is_subexpr:true, expr:(x + y))");
}

TEST_CASE("PatternCl")
{
    simple::ast::BinaryOp* expr = new simple::ast::BinaryOp {};
    simple::ast::VarRef* lhs = new simple::ast::VarRef {};
    lhs->name = "x";
    simple::ast::VarRef* rhs = new simple::ast::VarRef {};
    rhs->name = "y";
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = "+";
    ExprSpec expr_spec { true, std::unique_ptr<simple::ast::Expr>(expr) };

    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };

    auto ent = DeclaredEnt {};
    ent.declaration = declaration;

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = declaration;
    assign_pattern_cond->ent = &ent;
    assign_pattern_cond->expr_spec = std::move(expr_spec);
    auto pattern_cl = pql::ast::PatternCl {};
    pattern_cl.pattern_conds.push_back(std::move(assign_pattern_cond));

    INFO(pattern_cl.toString());
    REQUIRE(pattern_cl.toString() == "PatternCl[\n"
                                     "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
                                     "assignment_declaration:Declaration(ent:assign, name:foo), expr_spec:ExprSpec"
                                     "(is_subexpr:true, expr:(x + y)))\n"
                                     "]");
}

TEST_CASE("SuchThat")
{
    /** Initialize entity declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration1;
    pql::ast::EntRef* ent = &declared_ent;

    /** Initialize stmt declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration2);

    /** Initialize relationship */
    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent;
    relationship->modifier = stmt;

    pql::ast::SuchThatCl such_that_cl {};
    such_that_cl.rel_conds.push_back(std::move(relationship));
    INFO(such_that_cl.toString());
    REQUIRE(such_that_cl.toString() == "SuchThatCl[\n"
                                       "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                                       "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
                                       "]");
}

TEST_CASE("Select")
{
    /** Initialize such that */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent1;
    declared_ent1.declaration = declaration1;
    pql::ast::EntRef* ent1 = &declared_ent1;

    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration2);

    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent1;
    relationship->modifier = stmt;

    pql::ast::SuchThatCl such_that_cl {};
    such_that_cl.rel_conds.push_back(std::move(relationship));

    /** Initialize pattern*/
    simple::ast::BinaryOp* expr = new simple::ast::BinaryOp {};
    simple::ast::VarRef* lhs = new simple::ast::VarRef {};
    lhs->name = "x";
    simple::ast::VarRef* rhs = new simple::ast::VarRef {};
    rhs->name = "y";
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = "+";
    ExprSpec expr_spec { true, std::unique_ptr<simple::ast::Expr>(expr) };

    auto declaration3 = new Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };

    auto ent2 = DeclaredEnt {};
    ent2.declaration = declaration3;

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = declaration3;
    assign_pattern_cond->ent = &ent2;
    assign_pattern_cond->expr_spec = std::move(expr_spec);

    pql::ast::PatternCl pattern_cl {};
    pattern_cl.pattern_conds.push_back(std::move(assign_pattern_cond));

    pql::ast::Select select { std::move(such_that_cl), std::move(pattern_cl), declaration1 };
    INFO(select.toString());
    REQUIRE(select.toString() == "Select(such_that:SuchThatCl[\n"
                                 "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                                 "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
                                 "], pattern:PatternCl[\n"
                                 "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
                                 "assignment_declaration:Declaration(ent:assign, name:foo), expr_spec:ExprSpec"
                                 "(is_subexpr:true, expr:(x + y)))\n"
                                 "], ent:Declaration(ent:assign, name:foo))");
}

TEST_CASE("Query")
{
    /** Initialize such that */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent1;
    declared_ent1.declaration = declaration1;
    pql::ast::EntRef* ent1 = &declared_ent1;

    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(declaration2);

    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent1;
    relationship->modifier = stmt;

    pql::ast::SuchThatCl such_that_cl {};
    such_that_cl.rel_conds.push_back(std::move(relationship));

    /** Initialize pattern*/
    simple::ast::BinaryOp* expr = new simple::ast::BinaryOp {};
    simple::ast::VarRef* lhs = new simple::ast::VarRef {};
    lhs->name = "x";
    simple::ast::VarRef* rhs = new simple::ast::VarRef {};
    rhs->name = "y";
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->op = "+";
    ExprSpec expr_spec { true, std::unique_ptr<simple::ast::Expr>(expr) };

    auto declaration3 = new Declaration { "buzz", pql::ast::DESIGN_ENT::ASSIGN };

    auto ent2 = DeclaredEnt {};
    ent2.declaration = declaration3;

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = declaration3;
    assign_pattern_cond->ent = &ent2;
    assign_pattern_cond->expr_spec = std::move(expr_spec);
    pql::ast::PatternCl pattern_cl {};
    pattern_cl.pattern_conds.push_back(std::move(assign_pattern_cond));

    pql::ast::Select select { std::move(such_that_cl), std::move(pattern_cl), declaration1 };

    pql::ast::DeclarationList declaration_list {};
    declaration_list.addDeclaration("foo", declaration1);
    declaration_list.addDeclaration("bar", declaration2);
    declaration_list.addDeclaration("buzz", declaration3);

    pql::ast::Query query {};
    query.select = std::move(select);
    query.declarations = std::move(declaration_list);

    INFO(query.toString());

    constexpr auto expected = "Query(select:Select(such_that:SuchThatCl[\n"
                              "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                              "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
                              "], pattern:PatternCl[\n"
                              "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:buzz)), "
                              "assignment_declaration:Declaration(ent:assign, name:buzz), expr_spec:ExprSpec"
                              "(is_subexpr:true, expr:(x + y)))\n"
                              "], ent:Declaration(ent:assign, name:foo)), declarations:DeclarationList[\n"
                              "\tname:bar, declaration:Declaration(ent:assign, name:bar)\n"
                              "\tname:buzz, declaration:Declaration(ent:assign, name:buzz)\n"
                              "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
                              "])";

    REQUIRE(query.toString() == expected);
}
