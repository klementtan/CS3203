// test_ast.cpp
//
// Unit test for pql/ast.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/ast.h"
#include <iostream>
#include <unordered_map>
#include "util.h"

static void require(bool b)
{
    REQUIRE(b);
}

TEST_CASE("Declaration")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    require(declaration->toString() == "Declaration(ent:assign, name:foo)");
}

TEST_CASE("DeclarationList")
{
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::PROCEDURE };
    std::unordered_map<std::string, pql::ast::Declaration*> declarations = { { "foo", declaration1 },
        { "bar", declaration2 } };
    pql::ast::DeclarationList* declaration_list = new pql::ast::DeclarationList { declarations };
    require(declaration_list->toString() == "DeclarationList[\n"
                                            "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
                                            "\tname:bar, declaration:Declaration(ent:procedure, name:bar)\n"
                                            "]");
}

TEST_CASE("DeclaredStmt")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration;
    pql::ast::StmtRef* stmt = &delcared_stmt;
    require(stmt->toString() == "DeclaredStmt(declaration: Declaration(ent:assign, name:foo))");
}

TEST_CASE("StmtId")
{
    pql::ast::StmtId stmt_id;
    stmt_id.id = 1;
    pql::ast::StmtRef* stmt = &stmt_id;
    require(stmt->toString() == "StmtId(id:1)");
}

TEST_CASE("AllStmt")
{
    pql::ast::AllStmt all_stmt;
    pql::ast::StmtRef* stmt = &all_stmt;
    require(stmt->toString() == "AllStmt(name: _)");
}

TEST_CASE("DeclaredEnt")
{
    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration;
    pql::ast::EntRef* ent = &declared_ent;
    require(ent->toString() == "DeclaredEnt(declaration:Declaration(ent:assign, name:foo))");
}

TEST_CASE("EntName")
{
    pql::ast::EntName ent_name;
    ent_name.name = "foo";
    pql::ast::EntRef* ent = &ent_name;
    require(ent->toString() == "EntName(name:foo)");
}

TEST_CASE("AllEnt")
{
    pql::ast::AllEnt all_ent;
    pql::ast::EntRef* ent = &all_ent;
    INFO(ent->toString());
    require(ent->toString() == "AllEnt(name: _)");
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
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration2;
    pql::ast::StmtRef* stmt = &delcared_stmt;

    /** Initialize relationship */
    pql::ast::ModifiesS relationship;
    relationship.ent = ent;
    relationship.modifier = stmt;
    INFO(relationship.toString())
    require(relationship.toString() ==
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
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration2;
    pql::ast::StmtRef* stmt = &delcared_stmt;

    /** Initialize relationship */
    pql::ast::UsesS relationship;
    relationship.ent = ent;
    relationship.user = stmt;
    INFO(relationship.toString());
    require(relationship.toString() == "UsesS(user: DeclaredStmt(declaration: Declaration(ent:assign, name:bar)), "
                                       "ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))");
}

TEST_CASE("Parent")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt1;
    delcared_stmt1.declaration = declaration1;
    pql::ast::StmtRef* stmt1 = &delcared_stmt1;

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt2;
    delcared_stmt2.declaration = declaration2;
    pql::ast::StmtRef* stmt2 = &delcared_stmt2;

    /** Initialize relationship */
    pql::ast::Parent relationship;
    relationship.parent = stmt1;
    relationship.child = stmt2;
    INFO(relationship.toString());
    require(relationship.toString() == "Parent(parent:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)),"
                                       " child:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("ParentT")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt1;
    delcared_stmt1.declaration = declaration1;
    pql::ast::StmtRef* stmt1 = &delcared_stmt1;

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt2;
    delcared_stmt2.declaration = declaration2;
    pql::ast::StmtRef* stmt2 = &delcared_stmt2;

    /** Initialize relationship */
    pql::ast::ParentT relationship;
    relationship.ancestor = stmt1;
    relationship.descendant = stmt2;
    INFO(relationship.toString());
    require(relationship.toString() == "ParentT(ancestor:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)), "
                                       "descendant:DeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("Follows")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt1;
    delcared_stmt1.declaration = declaration1;
    pql::ast::StmtRef* stmt1 = &delcared_stmt1;

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt2;
    delcared_stmt2.declaration = declaration2;
    pql::ast::StmtRef* stmt2 = &delcared_stmt2;

    /** Initialize relationship */
    pql::ast::Follows relationship;
    relationship.directly_before = stmt1;
    relationship.directly_after = stmt2;
    INFO(relationship.toString());
    require(relationship.toString() == "Follows(directly_before:DeclaredStmt(declaration: Declaration(ent:assign, "
                                       "name:foo)), directly_after:DeclaredStmt(declaration: Declaration(ent:assign, "
                                       "name:bar)))");
}

TEST_CASE("FollowsT")
{
    /** Initialize stmt1 declaration */
    pql::ast::Declaration* declaration1 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt1;
    delcared_stmt1.declaration = declaration1;
    pql::ast::StmtRef* stmt1 = &delcared_stmt1;

    /** Initialize stmt2 declaration */
    pql::ast::Declaration* declaration2 = new pql::ast::Declaration { "bar", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredStmt delcared_stmt2;
    delcared_stmt2.declaration = declaration2;
    pql::ast::StmtRef* stmt2 = &delcared_stmt2;

    /** Initialize relationship */
    pql::ast::FollowsT relationship;
    relationship.before = stmt1;
    relationship.after = stmt2;
    INFO(relationship.toString());
    require(relationship.toString() == "FollowsT(before:DeclaredStmt(declaration: Declaration(ent:assign, name:foo)), "
                                       "afterDeclaredStmt(declaration: Declaration(ent:assign, name:bar)))");
}

TEST_CASE("ExprSpec")
{
    pql::ast::ExprSpec* expr_spec = new pql::ast::ExprSpec { true, true, "x+y" };

    require(expr_spec->toString() == "ExprSpec(any_before:true, any_after:true, expr:x+y)");
}

TEST_CASE("PatternCl")
{
    pql::ast::ExprSpec* expr_spec = new pql::ast::ExprSpec { true, true, "x+y" };

    pql::ast::Declaration* declaration = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent;
    declared_ent.declaration = declaration;
    pql::ast::EntRef* ent = &declared_ent;

    pql::ast::AssignPatternCond* assign_pattern_cond = new pql::ast::AssignPatternCond {};
    assign_pattern_cond->assignment_declaration = declaration;
    assign_pattern_cond->ent = ent;
    assign_pattern_cond->expr_spec = expr_spec;
    pql::ast::PatternCl* pattern_cl = new pql::ast::PatternCl { { assign_pattern_cond } };
    INFO(pattern_cl->toString());
    require(pattern_cl->toString() == "PatternCl[\n"
                                      "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
                                      "assignment_declaration:Declaration(ent:assign, name:foo), expr_spec:ExprSpec"
                                      "(any_before:true, any_after:true, expr:x+y))\n"
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
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration2;
    pql::ast::StmtRef* stmt = &delcared_stmt;

    /** Initialize relationship */
    pql::ast::ModifiesS relationship;
    relationship.ent = ent;
    relationship.modifier = stmt;

    pql::ast::SuchThatCl such_that_cl = { { &relationship } };
    INFO(such_that_cl.toString());
    require(such_that_cl.toString() == "SuchThatCl[\n"
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
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration2;
    pql::ast::StmtRef* stmt = &delcared_stmt;

    pql::ast::ModifiesS relationship;
    relationship.ent = ent1;
    relationship.modifier = stmt;

    pql::ast::SuchThatCl* such_that_cl = new pql::ast::SuchThatCl { { &relationship } };

    /** Initialize pattern*/
    pql::ast::ExprSpec* expr_spec = new pql::ast::ExprSpec { true, true, "x+y" };
    pql::ast::Declaration* declaration3 = new pql::ast::Declaration { "foo", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent2;
    declared_ent2.declaration = declaration3;
    pql::ast::EntRef* ent2 = &declared_ent2;
    pql::ast::AssignPatternCond* assign_pattern_cond = new pql::ast::AssignPatternCond {};
    assign_pattern_cond->assignment_declaration = declaration3;
    assign_pattern_cond->ent = ent2;
    assign_pattern_cond->expr_spec = expr_spec;
    pql::ast::PatternCl* pattern_cl = new pql::ast::PatternCl { { assign_pattern_cond } };

    pql::ast::Select select { such_that_cl, pattern_cl, declaration1 };
    INFO(select.toString());
    require(select.toString() == "Select(such_that:SuchThatCl[\n"
                                 "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                                 "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
                                 "], pattern:PatternCl[\n"
                                 "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)), "
                                 "assignment_declaration:Declaration(ent:assign, name:foo), expr_spec:ExprSpec"
                                 "(any_before:true, any_after:true, expr:x+y))\n"
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
    pql::ast::DeclaredStmt delcared_stmt;
    delcared_stmt.declaration = declaration2;
    pql::ast::StmtRef* stmt = &delcared_stmt;

    pql::ast::ModifiesS relationship;
    relationship.ent = ent1;
    relationship.modifier = stmt;

    pql::ast::SuchThatCl* such_that_cl = new pql::ast::SuchThatCl { { &relationship } };

    /** Initialize pattern*/
    pql::ast::ExprSpec* expr_spec = new pql::ast::ExprSpec { true, true, "x+y" };
    pql::ast::Declaration* declaration3 = new pql::ast::Declaration { "buz", pql::ast::DESIGN_ENT::ASSIGN };
    pql::ast::DeclaredEnt declared_ent2;
    declared_ent2.declaration = declaration3;
    pql::ast::EntRef* ent2 = &declared_ent2;
    pql::ast::AssignPatternCond* assign_pattern_cond = new pql::ast::AssignPatternCond {};
    assign_pattern_cond->assignment_declaration = declaration3;
    assign_pattern_cond->ent = ent2;
    assign_pattern_cond->expr_spec = expr_spec;
    pql::ast::PatternCl* pattern_cl = new pql::ast::PatternCl { { assign_pattern_cond } };

    pql::ast::Select select { such_that_cl, pattern_cl, declaration1 };

    std::unordered_map<std::string, pql::ast::Declaration*> declarations = { { "foo", declaration1 },
        { "bar", declaration2 }, { "buzz", declaration3 } };

    pql::ast::DeclarationList declaration_list { declarations };

    pql::ast::Query query { &select, &declaration_list };
    INFO(query.toString());
    require(query.toString() == "Query(select:Select(such_that:SuchThatCl[\n"
                                "\tModifiesS(modifier:DeclaredStmt(declaration: Declaration(ent:assign, name:"
                                "bar)), ent:DeclaredEnt(declaration:Declaration(ent:assign, name:foo)))\n"
                                "], pattern:PatternCl[\n"
                                "\tPatternCl(ent:DeclaredEnt(declaration:Declaration(ent:assign, name:buz)), "
                                "assignment_declaration:Declaration(ent:assign, name:buz), expr_spec:ExprSpec"
                                "(any_before:true, any_after:true, expr:x+y))\n"
                                "], ent:Declaration(ent:assign, name:foo)), declarations:DeclarationList[\n"
                                "\tname:foo, declaration:Declaration(ent:assign, name:foo)\n"
                                "\tname:bar, declaration:Declaration(ent:assign, name:bar)\n"
                                "\tname:buzz, declaration:Declaration(ent:assign, name:buz)\n"
                                "])");
}
