// test_uses.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto test_program = "procedure main {\n"
                                    "    flag = 0;\n"             // 1
                                    "    call computeCentroid;\n" // 2
                                    "    call printResults;\n"    // 3
                                    "}\n"
                                    "procedure readPoint {\n"
                                    "    read x;\n" // 4
                                    "    read y;\n" // 5
                                    "}\n"
                                    "procedure printResults {\n"
                                    "    print flag;\n"   // 6
                                    "    print cenX;\n"   // 7
                                    "    print cenY;\n"   // 8
                                    "    print normSq;\n" // 9
                                    "}\n"
                                    "procedure computeCentroid {\n"
                                    "    count = 0;\n"                     // 10
                                    "    cenX = 0;\n"                      // 11
                                    "    cenY = 0;\n"                      // 12
                                    "    call readPoint;\n"                // 13
                                    "    while ((x != 0) && (y != 0)) {\n" // 14
                                    "        count = count + 1;\n"         // 15
                                    "        cenX = cenX + x;\n"           // 16
                                    "        cenY = cenY + y;\n"           // 17
                                    "        call readPoint;\n"            // 18
                                    "    }\n"
                                    "    if (count == 0) then {\n" // 19
                                    "        flag = 1;\n"          // 20
                                    "    } else {\n"
                                    "        cenX = cenX / count;\n" // 21
                                    "        cenY = cenY / count;\n" // 22
                                    "    }\n"
                                    "    normSq = cenX * cenX + cenY * cenY;\n" // 23
                                    "}\n";

TEST_CASE("UsesP(Name, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Uses(\"main\", \"flag\")");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
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
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
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
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
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
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
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
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
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
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("main"));
    REQUIRE(result_s.count("printResults"));
    REQUIRE(result_s.count("computeCentroid"));
}




TEST_CASE("UsesS(StmtId, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p; Select p such that Uses(2, \"x\")");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 4);
    REQUIRE(result_s.count("6"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("9"));
}

TEST_CASE("UsesS(StmtId, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Uses(14, v)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 5);
    REQUIRE(result_s.count("x"));
    REQUIRE(result_s.count("y"));
    REQUIRE(result_s.count("cenX"));
    REQUIRE(result_s.count("cenY"));
    REQUIRE(result_s.count("count"));
}

TEST_CASE("UsesS(StmtId, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v; Select v such that Uses(18, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    CHECK_THROWS_WITH(eval.evaluate(), Catch::Matchers::Contains("is always false"));
}

TEST_CASE("UsesS(Decl, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("assign a;\nSelect a such that Uses(a, \"cenX\")");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("16"));
    REQUIRE(result_s.count("21"));
    REQUIRE(result_s.count("23"));
}

TEST_CASE("UsesS(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i; variable v;\nSelect v such that Uses(i, v)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("cenX"));
    REQUIRE(result_s.count("cenY"));
    REQUIRE(result_s.count("count"));
}

TEST_CASE("UsesS(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("assign a;\nSelect a such that Uses(a, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 6);
    REQUIRE(result_s.count("15"));
    REQUIRE(result_s.count("16"));
    REQUIRE(result_s.count("17"));
    REQUIRE(result_s.count("21"));
    REQUIRE(result_s.count("22"));
    REQUIRE(result_s.count("23"));
}

TEST_CASE("Uses(_, *)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Uses(_, v)");
    auto eval = pql::eval::Evaluator(pkb, query);

    CHECK_THROWS_WITH(eval.evaluate(), Catch::Matchers::Contains("first argument of Uses cannot be '_'"));
}


TEST_CASE("no follows")
{
    auto prog = simple::parser::parseProgram("procedure a { x = 1; }");
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Follows(_, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    CHECK_THROWS_WITH(eval.evaluate(), Catch::Matchers::Contains("always evaluate to false"));
}

TEST_CASE("no parent")
{
    auto prog = simple::parser::parseProgram("procedure a { x = 1; }");
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Parent(_, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    CHECK_THROWS_WITH(eval.evaluate(), Catch::Matchers::Contains("always false"));
}
