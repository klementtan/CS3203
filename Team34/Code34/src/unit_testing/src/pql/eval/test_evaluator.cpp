// test_ast.cpp
//
// Unit test for pql/eval/evaluator.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/parser.h"
#include "pql/eval/evaluator.h"
#include "pkb.h"
#include "simple/parser.h"
#include <list>


TEST_CASE("No such that")
{
    constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

    auto prog = simple::parser::parseProgram(in);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    pql::ast::Query* query = pql::parser::parsePQL("stmt a;\n"
                                                   "Select a");
    auto eval = new pql::eval::Evaluator(pkb, query);
    std::list<std::string> result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());
    REQUIRE(result_s.count("1"));
    REQUIRE(result_s.count("2"));
    REQUIRE(result_s.count("3"));
}

TEST_CASE("Follows clause")
{
    SECTION("Follows(DeclaredStmt, AllStmt)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        // Follows(DeclaredStmt, AllStmt)
        pql::ast::Query* query = pql::parser::parsePQL("stmt a;\n"
                                                       "Select a such that Follows (a,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("1"));
        REQUIRE(result_s.count("2"));
    }

    SECTION("Follows(AllStmt, DeclaredStmt)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();
        // Follows(AllStmt, DeclaredStmt)
        auto query = pql::parser::parsePQL("stmt a;\n"
                                           "Select a such that Follows (_,a)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
    SECTION("Follows(AllStmt, AllStmt)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();
        // Follows(AllStmt, DeclaredStmt)
        auto query = pql::parser::parsePQL("stmt a;\n"
                                           "Select a such that Follows (_,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 3);
        REQUIRE(result_s.count("1"));
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
    SECTION("Follows(StmtId, DeclaredStmt)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();
        auto query = pql::parser::parsePQL("stmt a;\n"
                                           "Select a such that Follows (1,a)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 1);
        REQUIRE(result_s.count("2"));
    }
    SECTION("Follows(DeclaredStmt, StmtId)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        // Follows(DeclaredStmt, StmtId)
        auto query = pql::parser::parsePQL("stmt a;\n"
                                           "Select a such that Follows (a,2)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 1);
        REQUIRE(result_s.count("1"));
    }
    SECTION("Follows(DeclaredStmt, DeclaredStmt)")
    {
        constexpr const auto in = R"(
            procedure A {
	            a = 1;
	            b = 1;
	            c = 1;
            }
        )";

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        auto query = pql::parser::parsePQL("stmt a, b;\n"
                                           "Select b such that Follows (a,b)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
}
