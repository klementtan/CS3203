// test_parent.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto test_program = "procedure main {\n"
                                    "    flag = 0;\n"                           // 1
                                    "    call computeCentroid;\n"               // 2
                                    "    call printResults;\n"                  // 3
                                    "}\n"                                       //
                                    "procedure readPoint {\n"                   //
                                    "    read x;\n"                             // 4
                                    "    read y;\n"                             // 5
                                    "}\n"                                       //
                                    "procedure printResults {\n"                //
                                    "    print flag;\n"                         // 6
                                    "    print cenX;\n"                         // 7
                                    "    print cenY;\n"                         // 8
                                    "    print normSq;\n"                       // 9
                                    "}\n"                                       //
                                    "procedure computeCentroid {\n"             //
                                    "    count = 0;\n"                          // 10
                                    "    cenX = 0;\n"                           // 11
                                    "    cenY = 0;\n"                           // 12
                                    "    call readPoint;\n"                     // 13
                                    "    while ((x != 0) && (y != 0)) {\n"      // 14
                                    "        count = count + 1;\n"              // 15
                                    "        cenX = cenX + x;\n"                // 16
                                    "        cenY = cenY + y;\n"                // 17
                                    "        call readPoint;\n"                 // 18
                                    "    }\n"                                   //
                                    "    if (count == 0) then {\n"              // 19
                                    "        flag = 1;\n"                       // 20
                                    "    } else {\n"                            //
                                    "        cenX = cenX / count;\n"            // 21
                                    "        cenY = cenY / count;\n"            // 22
                                    "    }\n"                                   //
                                    "    normSq = cenX * cenX + cenY * cenY;\n" // 23
                                    "    if (uwu == owo) then {\n"              // 24
                                    "        print kekw;\n"                     // 25
                                    "    } else {\n"                            //
                                    "        print asdf;\n"                     // 26
                                    "    }\n"
                                    "}\n";

TEST_CASE("Parent(StmtId, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent(19, 22)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 6);
    REQUIRE(result_s.count("6"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("9"));
    REQUIRE(result_s.count("25"));
    REQUIRE(result_s.count("26"));
}

TEST_CASE("Parent(StmtId, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("assign a;\nSelect a such that Parent(19, a)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("20"));
    REQUIRE(result_s.count("21"));
    REQUIRE(result_s.count("22"));
}

TEST_CASE("Parent(StmtId, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent(19, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 6);
    REQUIRE(result_s.count("6"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("9"));
    REQUIRE(result_s.count("25"));
    REQUIRE(result_s.count("26"));
}




TEST_CASE("Parent(Decl, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i;\nSelect i such that Parent(i, 26)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 1);
    REQUIRE(result_s.count("24"));
}

TEST_CASE("Parent(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i; assign a;\nSelect i such that Parent(i, a)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 1);
    REQUIRE(result_s.count("19"));
}

TEST_CASE("Parent(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i;\nSelect i such that Parent(i, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("19"));
    REQUIRE(result_s.count("24"));
}



TEST_CASE("Parent(_, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent(_, 21)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 6);
    REQUIRE(result_s.count("6"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("9"));
    REQUIRE(result_s.count("25"));
    REQUIRE(result_s.count("26"));
}

TEST_CASE("Parent(_, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent(_, p)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("25"));
    REQUIRE(result_s.count("26"));
}

TEST_CASE("Parent(_, _)")
{
    auto prog = simple::parser::parseProgram("procedure a { x = 1; }");
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("variable v;\nSelect v such that Parent(_, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    CHECK_THROWS_WITH(eval.evaluate(), Catch::Matchers::Contains("always false"));
}
