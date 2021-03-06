// test_parser.cpp
//
// Unit test for pql/parser.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/parser.h"


TEST_CASE("Basic Query")
{
    auto query = pql::parser::parsePQL("variable v;\n"
                                       "Select v");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    auto v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.result.tuple().front().declaration() == v_declaration);
    REQUIRE(query->select.clauses.empty());
}

TEST_CASE("BOOLEAN as declaration name")
{
    auto query = pql::parser::parsePQL("variable BOOLEAN;\n"
                                       "Select BOOLEAN");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    REQUIRE(query->select.result.isBool() == false);
    REQUIRE(query->select.result.tuple().size() == 1);
    REQUIRE(query->select.result.tuple().front().declaration()->name == "BOOLEAN");
}

TEST_CASE("variable as declaration name")
{
    auto query = pql::parser::parsePQL("variable variable;\n"
                                       "Select variable");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    REQUIRE(query->select.result.isBool() == false);
    REQUIRE(query->select.result.tuple().size() == 1);
    REQUIRE(query->select.result.tuple().front().declaration()->name == "variable");
}

TEST_CASE("Follows* Query")
{
    auto query = pql::parser::parsePQL("stmt s;\n"
                                       "Select s such that Follows* (6, s)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
    REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
    REQUIRE(s_declaration->name == "s");
    REQUIRE(query->select.result.tuple().front().declaration() == s_declaration);
    REQUIRE(query->select.clauses.size() == 1);

    auto follows_t = dynamic_cast<pql::ast::FollowsT*>(query->select.clauses[0].get());
    REQUIRE(follows_t != nullptr);

    CHECK(follows_t->before.id() == 6);
    CHECK(follows_t->after.declaration() == s_declaration);
}

TEST_CASE("ModifiesS Query")
{
    auto query = pql::parser::parsePQL("variable v;\n"
                                       "Select v such that Modifies (6, v)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.result.tuple().front().declaration() == v_declaration);
    REQUIRE(query->select.clauses.size() == 1);
    auto modifies_s = dynamic_cast<pql::ast::ModifiesS*>(query->select.clauses[0].get());
    REQUIRE(modifies_s != nullptr);

    CHECK(modifies_s->modifier.id() == 6);
    CHECK(modifies_s->ent.declaration() == v_declaration);
}

TEST_CASE("ModifiesP Query")
{
    auto query = pql::parser::parsePQL("variable v; procedure p;\n"
                                       "Select p such that Modifies (p, \"x\")");
    REQUIRE(query->declarations.getAllDeclarations().size() == 2);
    pql::ast::Declaration* p_declaration = query->declarations.getDeclaration("p");
    REQUIRE(p_declaration->design_ent == pql::ast::DESIGN_ENT::PROCEDURE);
    REQUIRE(p_declaration->name == "p");
    REQUIRE(query->select.result.tuple().front().declaration() == p_declaration);
    REQUIRE(query->select.clauses.size() == 1);
    auto modifies_p = dynamic_cast<pql::ast::ModifiesP*>(query->select.clauses[0].get());
    REQUIRE(modifies_p != nullptr);

    CHECK(modifies_p->modifier.declaration() == p_declaration);
    CHECK(modifies_p->ent.name() == "x");
}

TEST_CASE("UsesS Query")
{
    auto query = pql::parser::parsePQL("variable v;\n"
                                       "Select v such that Uses (14, v)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* v_declaration = query->declarations.getDeclaration("v");
    REQUIRE(v_declaration->design_ent == pql::ast::DESIGN_ENT::VARIABLE);
    REQUIRE(v_declaration->name == "v");
    REQUIRE(query->select.result.tuple().front().declaration() == v_declaration);
    REQUIRE(query->select.clauses.size() == 1);
    auto uses_s = dynamic_cast<pql::ast::UsesS*>(query->select.clauses[0].get());
    REQUIRE(uses_s != nullptr);

    CHECK(uses_s->user.id() == 14);
    CHECK(uses_s->ent.declaration() == v_declaration);
}

TEST_CASE("Pattern Query")
{
    auto query = pql::parser::parsePQL("assign a;\n"
                                       "Select a pattern a ( \"normSq\" , _\"cenX * cenX\"_)");
    REQUIRE(query->declarations.getAllDeclarations().size() == 1);
    pql::ast::Declaration* a_declaration = query->declarations.getDeclaration("a");
    REQUIRE(a_declaration->design_ent == pql::ast::DESIGN_ENT::ASSIGN);
    REQUIRE(a_declaration->name == "a");
    REQUIRE(query->select.result.tuple().front().declaration() == a_declaration);
    REQUIRE(query->select.clauses.size() == 1);
    auto assign_pattern_cond = dynamic_cast<pql::ast::AssignPatternCond*>(query->select.clauses[0].get());
    REQUIRE(assign_pattern_cond != nullptr);
    REQUIRE(assign_pattern_cond->assignment_declaration == a_declaration);

    REQUIRE(assign_pattern_cond->ent.name() == "normSq");

    auto& expr_spec = assign_pattern_cond->expr_spec;
    REQUIRE(expr_spec.is_subexpr == true);
    auto* expr = dynamic_cast<simple::ast::BinaryOp*>(expr_spec.expr.get());
    REQUIRE(expr);
    REQUIRE(expr->op == "*");
    auto* lhs = dynamic_cast<simple::ast::VarRef*>(expr->lhs.get());
    REQUIRE(lhs);
    REQUIRE(lhs->name == "cenX");
    auto* rhs = dynamic_cast<simple::ast::VarRef*>(expr->rhs.get());
    REQUIRE(rhs);
    REQUIRE(rhs->name == "cenX");
}

TEST_CASE("Parent Query")
{
    SECTION("Valid query")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select s such that Parent(6, s)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.result.tuple().front().declaration() == s_declaration);
        REQUIRE(query->select.clauses.size() == 1);
        auto parent = dynamic_cast<pql::ast::Parent*>(query->select.clauses[0].get());
        REQUIRE(parent != nullptr);

        CHECK(parent->parent.id() == 6);
        CHECK(parent->child.declaration() == s_declaration);
    }
    SECTION("Invalid query")
    {
        REQUIRE_THROWS_WITH(pql::parser::parsePQL("stmt s;\nSelect s such that Parentt(6, s)"),
            Catch::Contains("unexpected 't' after keyword 'Parent'"));
    }
}

TEST_CASE("Result Clause")
{
    SECTION("Tuple with Single Element no multi element syntax")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select s such that Follows(s,_)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 1);
        REQUIRE(query->select.result.tuple().front().declaration() == s_declaration);
    }
    SECTION("Tuple with Single AttrRef Element no multi element syntax")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select s.stmt# such that Follows(s,_)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 1);
        pql::ast::Elem elem = query->select.result.tuple().front();
        REQUIRE(elem.isAttrRef());
        REQUIRE(elem.attrRef().decl == s_declaration);
        REQUIRE(elem.attrRef().attr_name == pql::ast::AttrName::kStmtNum);
    }
    SECTION("Tuple with Single AttrRef Element no multi element syntax and weird spacing")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select s     .      stmt# such that Follows(s,_)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 1);
        pql::ast::Elem elem = query->select.result.tuple().front();
        REQUIRE(elem.isAttrRef());
        REQUIRE(elem.attrRef().decl == s_declaration);
        REQUIRE(elem.attrRef().attr_name == pql::ast::AttrName::kStmtNum);
    }
    SECTION("BOOLEAN result clause")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select BOOLEAN such that Follows(s,_)");
        REQUIRE(query->select.result.isBool());
    }
    SECTION("Tuple with Single Element using multi element syntax")
    {
        auto query = pql::parser::parsePQL("stmt s;\n"
                                           "Select <s> such that Follows(s,_)");
        REQUIRE(query->declarations.getAllDeclarations().size() == 1);
        pql::ast::Declaration* s_declaration = query->declarations.getDeclaration("s");
        REQUIRE(s_declaration->design_ent == pql::ast::DESIGN_ENT::STMT);
        REQUIRE(s_declaration->name == "s");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 1);
        REQUIRE(query->select.result.tuple().front().declaration() == s_declaration);
    }
    SECTION("Tuple with Multiple Element")
    {
        auto query = pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                           "Select <s1,s2,s3.stmt#> such that Follows(s1,_)");
        pql::ast::Declaration* s1_declaration = query->declarations.getDeclaration("s1");
        pql::ast::Declaration* s2_declaration = query->declarations.getDeclaration("s2");
        pql::ast::Declaration* s3_declaration = query->declarations.getDeclaration("s3");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 3);
        REQUIRE(query->select.result.tuple()[0].declaration() == s1_declaration);
        REQUIRE(query->select.result.tuple()[1].declaration() == s2_declaration);
        REQUIRE(query->select.result.tuple()[2].attrRef().decl == s3_declaration);
        REQUIRE(query->select.result.tuple()[2].attrRef().attr_name == pql::ast::AttrName::kStmtNum);
    }
    SECTION("Valid weird white spaces")
    {
        auto query = pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                           "Select      <   s1    , s2 ,     s3   > such that Follows(s1,_)");
        pql::ast::Declaration* s1_declaration = query->declarations.getDeclaration("s1");
        pql::ast::Declaration* s2_declaration = query->declarations.getDeclaration("s2");
        pql::ast::Declaration* s3_declaration = query->declarations.getDeclaration("s3");
        REQUIRE(query->select.result.isTuple());
        REQUIRE(query->select.result.tuple().size() == 3);
        REQUIRE(query->select.result.tuple()[0].declaration() == s1_declaration);
        REQUIRE(query->select.result.tuple()[1].declaration() == s2_declaration);
        REQUIRE(query->select.result.tuple()[2].declaration() == s3_declaration);
    }

    SECTION("valid keyword synonyms")
    {
        using namespace pql::parser;

        CHECK(parsePQL("assign assign; Select assign pattern assign(_, _)")->isValid());
        CHECK(parsePQL("assign pattern; Select pattern pattern pattern(_, _)")->isValid());
        CHECK(parsePQL("stmt Select; Select Select such that Follows(Select, Select)")->isValid());
        CHECK(parsePQL("stmt such, that, with, stmt; Select <such, that, with, stmt>"
                       " such that Follows(_,_)")
                  ->isValid());
    }

    SECTION("Invalid ResultCl")
    {
        // comma before first element in tuple
        CHECK_THROWS_WITH(pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                                "Select <,s1,s2,s3> such that Follows(s1,_)"),
            "expected identifier, found ',' instead");
        // No comma between elements
        CHECK_THROWS_WITH(pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                                "Select <s1 s2,s3> such that Follows(s1,_)"),
            Catch::Contains("expected either ',' or '>' in tuple, found 's2' instead"));
        // No ending '>'
        CHECK_THROWS_WITH(pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                                "Select <s1,s2,s3"),
            Catch::Contains("expected either ',' or '>' in tuple, found '$end of input' instead"));
        // No empty tuple
        CHECK_THROWS_WITH(pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                                "Select <>"),
            Catch::Contains("expected identifier, found '>' instead"));

        // Space between 'stmt' and '#'
        CHECK_THROWS_WITH(pql::parser::parsePQL("stmt s1, s2, s3;\n"
                                                "Select s1.stmt #"),
            Catch::Contains("Invalid attribute 'stmt'"));
    }
}


#define TEST_VALID(query) CHECK(pql::parser::parsePQL(query)->isValid())
#define TEST_INVALID(query) CHECK(pql::parser::parsePQL(query)->isInvalid())


TEST_CASE("Next/*")
{
    using namespace pql::parser;

    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(s1, s2)");
    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(s1, _)");
    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(_, _)");
    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(_, s2)");
    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(69, 420)");
    TEST_VALID("stmt s1, s2; Select <s1, s2> such that Next(69, s1)");
}




TEST_CASE("invalid queries")
{
    using namespace pql::parser;

    SECTION("duplicate declarations")
    {
        auto query = "stmt s; assign s; Select s";
        auto q = parsePQL(query);
        REQUIRE(q->isInvalid());
    }

    SECTION("wrong types")
    {
        CHECK_THROWS_WITH(parsePQL("stmt s; Select BOOLEAN such that Next(s, \"asdf\")"),
            Catch::Contains("Invalid stmt ref starting with asdf"));
    }

    SECTION("such-that/parent*/follows* spacing")
    {
        CHECK_THROWS_WITH(parsePQL("stmt s ; Select s   such    that   Parent(s, _)"),
            Catch::Contains("unexpected token 'such' in Select"));

        CHECK_THROWS_WITH(
            parsePQL("stmt s; Select s such that Follows  *  (s, _)"), Catch::Contains("invalid token '*'"));

        CHECK_THROWS_WITH(
            parsePQL("stmt s; Select s such that Parent  *  (s, _)"), Catch::Contains("invalid token '*'"));
    }
}

TEST_CASE("valid multi-queries")
{
    TEST_VALID("stmt s; Select s such that Follows(s, s) and Parent*(s, s) and Follows*(s, s)");
    TEST_VALID("stmt s; Select s such that Follows(s, s) such that Parent*(s, s) such that Follows*(s, s)");

    TEST_VALID("stmt s; assign a; Select <a,s> pattern a(_, _) and a(_, _) and a(_, _)");
    TEST_VALID("stmt s; assign a; Select <a,s> pattern a(_, _) pattern a(_, _) pattern a(_, _)");

    TEST_VALID("stmt s; assign a; Select <a,s> pattern a(_, _) such that Follows(s, _) pattern a(_, _)");
    TEST_VALID("stmt s; assign a; Select <a,s> pattern a(_, _) such that Follows(s, _) with s.stmt# = a.stmt#");

    TEST_VALID("call c1; call c2; Select <c1,c2> with c1.stmt# = c2.stmt# and c1.procName = c2.procName");
    TEST_VALID("call c1; call c2; Select <c1,c2> with c1.stmt# = c2.stmt# with c1.procName = c2.procName");
    TEST_VALID("call c1; call c2; Select <c1,c2> with c1.stmt# = 69 and c1.procName = \"kekw\"");

    // only prog_lines can be used without attr_ref
    TEST_VALID("prog_line p1, p2; Select <p1,p2> with p1 = 69 and p1 = p2");

    TEST_INVALID("prog_line p1, p2; Select <p1,p2> with p1.stmt# = 69 and p1 = p2");
    TEST_INVALID("call c1; call c2; Select <c1,c2> with c1.stmt# = 69 and c1 = c2");
}
