// test_ast.cpp
//
// Unit test for pql/eval/evaluator.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include <iostream>

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
