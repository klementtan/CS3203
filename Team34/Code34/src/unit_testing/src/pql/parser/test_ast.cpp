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
    auto declaration = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    REQUIRE(declaration.toString() == "Declaration(ent:assign, name:foo)");
}

TEST_CASE("DeclarationList")
{
    auto declaration_list = pql::ast::DeclarationList {};
    declaration_list.addDeclaration("foo", DESIGN_ENT::ASSIGN);
    declaration_list.addDeclaration("bar", DESIGN_ENT::PROCEDURE);

    REQUIRE(declaration_list.toString() == "DeclarationList[\n"
                                           "\tname:bar, declaration:Declaration(ent:procedure, name:bar)\n"
                                           "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
                                           "]");
}

TEST_CASE("DeclaredStmt")
{
    auto declaration = Declaration { "foo", DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(&declaration);

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
    auto declaration = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent = EntRef::ofDeclaration(&declaration);
    REQUIRE(ent.toString() == "DeclaredEnt(declaration:Declaration(ent:assign, name:foo))");
}

TEST_CASE("EntName")
{
    auto ent = EntRef::ofName("foo");
    REQUIRE(ent.toString() == "EntName(name:foo)");
}

TEST_CASE("AllEnt")
{
    auto ent = EntRef::ofWildcard();
    REQUIRE(ent.toString() == "AllEnt(name: _)");
}

TEST_CASE("ModifiesS")
{
    /** Initialize entity declaration */
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent = EntRef::ofDeclaration(&declaration1);

    /** Initialize stmt declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(&declaration2);

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
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent = EntRef::ofDeclaration(&declaration1);

    /** Initialize stmt declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(&declaration2);

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
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(&declaration1);

    /** Initialize stmt2 declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(&declaration2);

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
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(&declaration1);

    /** Initialize stmt2 declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(&declaration2);

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
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(&declaration1);

    /** Initialize stmt2 declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(&declaration2);

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
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt1 = StmtRef::ofDeclaration(&declaration1);

    /** Initialize stmt2 declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt2 = StmtRef::ofDeclaration(&declaration2);

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
    auto expr = std::make_unique<simple::ast::BinaryOp>();
    auto lhs = std::make_unique<simple::ast::VarRef>();
    lhs->name = "x";
    auto rhs = std::make_unique<simple::ast::VarRef>();
    rhs->name = "y";
    expr->lhs = std::move(lhs);
    expr->rhs = std::move(rhs);
    expr->op = "+";
    auto expr_spec = pql::ast::ExprSpec { true, std::move(expr) };

    REQUIRE(expr_spec.toString() == "ExprSpec(is_subexpr:true, expr:(x + y))");
}

TEST_CASE("PatternCl")
{
    auto expr = std::make_unique<simple::ast::BinaryOp>();
    auto lhs = std::make_unique<simple::ast::VarRef>();
    lhs->name = "x";
    auto rhs = std::make_unique<simple::ast::VarRef>();
    rhs->name = "y";
    expr->lhs = std::move(lhs);
    expr->rhs = std::move(rhs);
    expr->op = "+";
    auto expr_spec = pql::ast::ExprSpec { true, std::move(expr) };

    auto declaration = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent = EntRef::ofDeclaration(&declaration);

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = &declaration;
    assign_pattern_cond->ent = ent;
    assign_pattern_cond->expr_spec = std::move(expr_spec);

    INFO(assign_pattern_cond->toString());
    REQUIRE(assign_pattern_cond->toString() ==
            "AssignPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
            "assignment_declaration:Declaration(ent:assign, name:foo), expr_spec:ExprSpec"
            "(is_subexpr:true, expr:(x + y)))");
}

TEST_CASE("SuchThat")
{
    /** Initialize entity declaration */
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent = EntRef::ofDeclaration(&declaration1);

    /** Initialize stmt declaration */
    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(&declaration2);

    /** Initialize relationship */
    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent;
    relationship->modifier = stmt;

    INFO(relationship->toString());
    REQUIRE(relationship->toString() == "ModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                                        "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))");
}

TEST_CASE("Select")
{
    /** Initialize such that */
    auto declaration1 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent1 = EntRef::ofDeclaration(&declaration1);

    auto declaration2 = Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    auto stmt = StmtRef::ofDeclaration(&declaration2);

    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent1;
    relationship->modifier = stmt;

    /** Initialize pattern*/
    auto expr = std::make_unique<simple::ast::BinaryOp>();
    auto lhs = std::make_unique<simple::ast::VarRef>();
    lhs->name = "x";
    auto rhs = std::make_unique<simple::ast::VarRef>();
    rhs->name = "y";
    expr->lhs = std::move(lhs);
    expr->rhs = std::move(rhs);
    expr->op = "+";
    auto expr_spec = pql::ast::ExprSpec { true, std::move(expr) };

    auto declaration3 = Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    auto ent2 = EntRef::ofDeclaration(&declaration3);

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = &declaration3;
    assign_pattern_cond->ent = ent2;
    assign_pattern_cond->expr_spec = std::move(expr_spec);

    auto result_cl = pql::ast::ResultCl::ofTuple({ pql::ast::Elem::ofDeclaration(&declaration1) });

    pql::ast::Select select {};
    select.clauses.push_back(std::move(relationship));
    select.clauses.push_back(std::move(assign_pattern_cond));
    select.result = std::move(result_cl);

    INFO(select.toString());
    REQUIRE(select.toString() == "Select(result:ResultCl(type: Tuple, tuple: [Elem(ref_type: Declaration, "
        "decl: Declaration(ent:assign, name:foo))], clauses:[\n"
        "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)), "
            "ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
        "\tAssignPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
            "assignment_declaration:Declaration(ent:assign, name:foo), "
            "expr_spec:ExprSpec(is_subexpr:true, expr:(x + y)))\n])");
}

TEST_CASE("Query")
{
    /** Initialize such that */

    pql::ast::DeclarationList declaration_list {};
    auto declaration1 = declaration_list.addDeclaration("foo", pql::ast::DESIGN_ENT::ASSIGN);
    auto declaration2 = declaration_list.addDeclaration("bar", pql::ast::DESIGN_ENT::ASSIGN);
    auto declaration3 = declaration_list.addDeclaration("buzz", pql::ast::DESIGN_ENT::ASSIGN);

    auto ent1 = EntRef::ofDeclaration(declaration1);
    auto stmt = StmtRef::ofDeclaration(declaration2);
    auto ent2 = EntRef::ofDeclaration(declaration3);

    auto relationship = std::make_unique<pql::ast::ModifiesS>();
    relationship->ent = ent1;
    relationship->modifier = stmt;

    /** Initialize pattern*/
    auto expr = std::make_unique<simple::ast::BinaryOp>();
    auto lhs = std::make_unique<simple::ast::VarRef>();
    lhs->name = "x";
    auto rhs = std::make_unique<simple::ast::VarRef>();
    rhs->name = "y";
    expr->lhs = std::move(lhs);
    expr->rhs = std::move(rhs);
    expr->op = "+";
    auto expr_spec = pql::ast::ExprSpec { true, std::move(expr) };

    auto assign_pattern_cond = std::make_unique<pql::ast::AssignPatternCond>();
    assign_pattern_cond->assignment_declaration = declaration3;
    assign_pattern_cond->ent = ent2;
    assign_pattern_cond->expr_spec = std::move(expr_spec);

    auto result_cl = pql::ast::ResultCl::ofTuple({ pql::ast::Elem::ofDeclaration(declaration1) });
    pql::ast::Select select {};
    select.clauses.push_back(std::move(relationship));
    select.clauses.push_back(std::move(assign_pattern_cond));
    select.result = std::move(result_cl);

    pql::ast::Query query {};
    query.select = std::move(select);
    query.declarations = std::move(declaration_list);

    INFO(query.toString());

    constexpr auto expected = "Query(select:Select(result:ResultCl(type: Tuple, tuple: [Elem(ref_type: Declaration, "
        "decl: Declaration(ent:assign, name:foo))], clauses:[\n"
            "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)), "
            "ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
            "\tAssignPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:buzz)), "
            "assignment_declaration:Declaration(ent:assign, name:buzz), expr_spec:ExprSpec(is_subexpr:true, expr:(x + y)))\n"
            "]), declarations:DeclarationList[\n"
            "\tname:bar, declaration:Declaration(ent:assign, name:bar)\n"
            "\tname:buzz, declaration:Declaration(ent:assign, name:buzz)\n"
            "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
            "])";

    REQUIRE(query.toString() == expected);
}
