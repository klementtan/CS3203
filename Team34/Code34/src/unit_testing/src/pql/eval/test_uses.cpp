// test_uses.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto test_program = R"(
    procedure main {
      flag = 0;
      call computeCentroid;
      call printResults;
    }
    procedure readPoint {
        read x;
        read y;
    }
    procedure printResults {
        print flag;
        print cenX;
        print cenY;
        print normSq;
    }
    procedure computeCentroid {
        count = 0;
        cenX = 0;
        cenY = 0;
        call readPoint;
        while ((x != 0) && (y != 0)) {
            count = count + 1;
            cenX = cenX + x;
            cenY = cenY + y;
            call readPoint;
        }
        if (count == 0) then {
            flag = 1;
        } else {
            cenX = cenX / count;
            cenY = cenY / count;
        }
        normSq = cenX * cenX + cenY * cenY;
    }
)";

TEST_CASE("UsesP(Name, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Uses(\"main\", \"flag\")");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 7);
    REQUIRE(result_s.count("x"));
    REQUIRE(result_s.count("y"));
    REQUIRE(result_s.count("count"));
    REQUIRE(result_s.count("cenX"));
    REQUIRE(result_s.count("cenY"));
    REQUIRE(result_s.count("flag"));
    REQUIRE(result_s.count("normSq"));
}


TEST_CASE("UsesP(Decl, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("procedure p;\nSelect p such that Uses(p, \"cenX\")");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("printResults"));
    REQUIRE(result_s.count("computeCentroid"));
}

TEST_CASE("UsesP(Name, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Uses(\"main\", v)");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 7);
    REQUIRE(result_s.count("x"));
    REQUIRE(result_s.count("y"));
    REQUIRE(result_s.count("count"));
    REQUIRE(result_s.count("cenX"));
    REQUIRE(result_s.count("cenY"));
    REQUIRE(result_s.count("flag"));
    REQUIRE(result_s.count("normSq"));
}

TEST_CASE("UsesP(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("procedure p; variable v;\nSelect p such that Uses(p, v)");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("main"));
    REQUIRE(result_s.count("printResults"));
    REQUIRE(result_s.count("computeCentroid"));
}

TEST_CASE("UsesP(Name, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("procedure p;\nSelect p such that Uses(\"printResults\", _)");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 4);
    REQUIRE(result_s.count("main"));
    REQUIRE(result_s.count("readPoint"));
    REQUIRE(result_s.count("printResults"));
    REQUIRE(result_s.count("computeCentroid"));
}

TEST_CASE("UsesP(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("procedure p;\nSelect p such that Uses(p, _)");
    auto eval = new pql::eval::Evaluator(pkb, query);

    auto result = eval->evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("main"));
    REQUIRE(result_s.count("printResults"));
    REQUIRE(result_s.count("computeCentroid"));
}
