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

TEST_CASE("Follows* clause")
{
    SECTION("Follows*(DeclaredStmt, AllStmt)")
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
                                                       "Select a such that Follows* (a,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("1"));
        REQUIRE(result_s.count("2"));
    }

    SECTION("Follows*(AllStmt, DeclaredStmt)")
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
                                           "Select a such that Follows* (_,a)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
    SECTION("Follows*(AllStmt, AllStmt)")
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
                                           "Select a such that Follows* (_,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 3);
        REQUIRE(result_s.count("1"));
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
    SECTION("Follows*(StmtId, DeclaredStmt)")
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
                                           "Select a such that Follows* (1,a)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
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
                                           "Select a such that Follows* (a,3)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("1"));
        REQUIRE(result_s.count("2"));
    }
    SECTION("Follows*(DeclaredStmt, DeclaredStmt)")
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
                                           "Select b such that Follows* (a,b)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        auto result = eval->evaluate();
        auto result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("2"));
        REQUIRE(result_s.count("3"));
    }
}


TEST_CASE("ModifiesP clause")
{
    SECTION("ModifiesP(DeclaredEnt, DeclaredEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("procedure p;\n"
                                                       "variable v;\n"
                                                       "Select p such that Modifies (p,v)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 3);
        REQUIRE(result_s.count("main"));
        REQUIRE(result_s.count("readPoint"));
        REQUIRE(result_s.count("computeCentroid"));
    }
    SECTION("ModifiesP(DeclaredEnt, DeclaredEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("procedure p;\n"
                                                       "Select p such that Modifies (p,\"flag\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("main"));
        REQUIRE(result_s.count("computeCentroid"));
    }
    SECTION("ModifiesP(DeclaredEnt, AllEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        // Follows(DeclaredStmt, AllStmt)
        pql::ast::Query* query = pql::parser::parsePQL("procedure p;\n"
                                                       "Select p such that Modifies (p,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 3);
        REQUIRE(result_s.count("main"));
        REQUIRE(result_s.count("readPoint"));
        REQUIRE(result_s.count("computeCentroid"));
    }

    SECTION("ModifiesP(EntName, DeclaredEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (\"readPoint\", v)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        REQUIRE(result_s.size() == 2);
        REQUIRE(result_s.count("x"));
        REQUIRE(result_s.count("y"));

        // TODO(#73): Test transitive property through call statement
        /**
        prog = simple::parser::parseProgram(in);
        pkb = pkb::processProgram(prog.unwrap()).unwrap();

        query = pql::parser::parsePQL("variable v;\n"
                                      "Select v such that Modifies (\"main\", v)");
        eval = new pql::eval::Evaluator(pkb, query);
        result = eval->evaluate();
        result_s = std::unordered_set<std::string>(result.begin(), result.end());
        REQUIRE(result_s.size() == 6);
        REQUIRE(result_s.count("x"));
        REQUIRE(result_s.count("y"));
        REQUIRE(result_s.count("count"));
        REQUIRE(result_s.count("cenX"));
        REQUIRE(result_s.count("cenY"));
        REQUIRE(result_s.count("flag"));
        REQUIRE(result_s.count("normSq"));
        */
    }
    SECTION("ModifiesP(EntName, AllEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (\"readPoint\", _)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
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
    SECTION("ModifiesP(EntName, AllEnt) always fail")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (\"printResults\", _)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        CHECK_THROWS_WITH(eval->evaluate(),
            Catch::Matchers::Contains("ModifiesP(modifier:EntName(name:printResults), ent:AllEnt(name: _)) always "
                                      "evaluate to false. printResults does not modify any variable."));
    }
    SECTION("ModifiesP(EntName, EntName)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (\"main\", \"flag\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
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
    SECTION("ModifiesP(EntName, EntName) always fail")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (\"readPoint\", \"flag\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        CHECK_THROWS_WITH(eval->evaluate(),
            Catch::Matchers::Contains("ModifiesP(modifier:EntName(name:readPoint), ent:EntName(name:flag)) always "
                                      "evaluate to false. readPoint does not modify flag."));
    }
}

TEST_CASE("ModifiesS(...,...)")
{
    SECTION("ModifiesS(DeclaredStmt, DeclaredEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("assign a;\n"
                                                       "variable v;\n"
                                                       "Select a such that Modifies (a,v)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "1", "10", "11", "12", "15", "16", "17", "20", "21", "22",
            "23" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }
    SECTION("ModifiesS(DeclaredStmt, EntName)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("assign a;\n"
                                                       "variable v;\n"
                                                       "Select a such that Modifies (a,\"flag\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "1", "20" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }
    SECTION("ModifiesS(DeclaredStmt, AllEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("call c;\n"
                                                       "Select c such that Modifies (c,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "2", "13", "18" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }
    SECTION("ModifiesS(StmtId, DeclaredStmt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (23,v)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "normSq" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }

    SECTION("ModifiesS(StmtId, AllEnt)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (23,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "normSq", "x", "y", "flag", "cenX", "cenY", "count" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }

    SECTION("ModifiesS(StmtId, EntName) fails with non modifying statement")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (3,_)");
        auto eval = new pql::eval::Evaluator(pkb, query);
        REQUIRE_THROWS_WITH(eval->evaluate(), "ModifiesS(modifier:StmtId(id:3), ent:AllEnt(name: _)) always evaluate "
                                              "to false. StatementNum 3 does not modify any variable.");
    }
    SECTION("ModifiesS(StmtId, EntName)")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (23,\"normSq\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        std::list<std::string> result = eval->evaluate();
        std::unordered_set<std::string> result_s(result.begin(), result.end());
        std::unordered_set<std::string> expected_result = { "normSq", "x", "y", "flag", "cenX", "cenY", "count" };
        REQUIRE(result_s.size() == expected_result.size());
        for(std::string acutal_stmt_num_str : result_s)
        {
            INFO(acutal_stmt_num_str);
            REQUIRE(expected_result.count(acutal_stmt_num_str));
        }
    }
    SECTION("ModifiesS(StmtId, EntName) fails with non modifying statement")
    {
        constexpr const auto in = R"(
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

        auto prog = simple::parser::parseProgram(in);
        auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

        pql::ast::Query* query = pql::parser::parsePQL("variable v;\n"
                                                       "Select v such that Modifies (23,\"x\")");
        auto eval = new pql::eval::Evaluator(pkb, query);
        REQUIRE_THROWS_WITH(eval->evaluate(), "StatementNum: ModifiesS(modifier:StmtId(id:23), ent:EntName(name:x)) "
                                              "always evaluate to false. 23 does not modify x.");
    }
}
