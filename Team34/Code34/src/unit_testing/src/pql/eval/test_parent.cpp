// test_parent.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

// clang-format off
constexpr const auto test_program_1 = " \
 procedure main {\n"                        //
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
// clang-format on


TEST_CASE("Parent(StmtId, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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
    auto prog = simple::parser::parseProgram(test_program_1);
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

// clang-format off
constexpr const auto test_program_2 = "\
 procedure foo {\n"
"    if (a == 1) then {\n"                  // 1
"        print x;\n"                        // 2
"        if (b == 2) then {\n"              // 3
"            print x;\n"                    // 4
"            read x;\n"                     // 5
"            call bar;\n"                   // 6
"        } else {\n"                        //
"            print x;\n"                    // 7
"            while (c == 3) {\n"            // 8
"                print x;\n"                // 9
"                if (d == 4) then {\n"      // 10
"                    print x;\n"            // 11
"                    q = 420;\n"            // 12
"                } else {\n"                //
"                    while (e == 5) {\n"    // 13
"                        print x;\n"        // 14
"                        p = 69;\n"         // 15
"                    }\n"                   //
"                    if (f == 6) then {\n"  // 16
"                        print x;\n"        // 17
"                        q = 3;\n"          // 18
"                        read r;\n"         // 19
"                    } else {\n"            //
"                        print x;\n"        // 20
"                        p = 1;\n"          // 21
"                        read r;\n"         // 22
"                    }\n"                   //
"                }\n"                       //
"            }\n"                           //
"        }\n"                               //
"    } else {\n"                            //
"        print x;\n"                        // 23
"    }\n"                                   //
"}\n"                                       //
"procedure bar {\n"                         //
"    x = 1;\n"                              // 24
"}\n";                                      //
// clang-format on


TEST_CASE("Parent*(StmtId, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent*(1, 11)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 9);
    REQUIRE(result_s.count("2"));
    REQUIRE(result_s.count("4"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("9"));
    REQUIRE(result_s.count("11"));
    REQUIRE(result_s.count("14"));
    REQUIRE(result_s.count("17"));
    REQUIRE(result_s.count("20"));
    REQUIRE(result_s.count("23"));
}

TEST_CASE("Parent*(StmtId, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("read r;\nSelect r such that Parent*(8, r)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("19"));
    REQUIRE(result_s.count("22"));
}

TEST_CASE("Parent*(StmtId, _)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("print p;\nSelect p such that Parent*(8, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 9);
    REQUIRE(result_s.count("2"));
    REQUIRE(result_s.count("4"));
    REQUIRE(result_s.count("7"));
    REQUIRE(result_s.count("9"));
    REQUIRE(result_s.count("11"));
    REQUIRE(result_s.count("14"));
    REQUIRE(result_s.count("17"));
    REQUIRE(result_s.count("20"));
    REQUIRE(result_s.count("23"));
}

TEST_CASE("Parent*(Decl, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i;\nSelect i such that Parent*(i, 20)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 4);
    REQUIRE(result_s.count("1"));
    REQUIRE(result_s.count("3"));
    REQUIRE(result_s.count("10"));
    REQUIRE(result_s.count("16"));
}

TEST_CASE("Parent*(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i; call c;\nSelect i such that Parent*(i, c)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("1"));
    REQUIRE(result_s.count("3"));
}

TEST_CASE("Parent*(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("while w;\nSelect w such that Parent*(w, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("13"));
}

TEST_CASE("Parent*(_, StmtId)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("while w;\nSelect w such that Parent*(_, 2)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("13"));
}

TEST_CASE("Parent*(_, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("if i;\nSelect i such that Parent*(_, i)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 3);
    REQUIRE(result_s.count("3"));
    REQUIRE(result_s.count("10"));
    REQUIRE(result_s.count("16"));
}

TEST_CASE("Parent*(_, _)")
{
    auto prog = simple::parser::parseProgram(test_program_2);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    auto query = pql::parser::parsePQL("while w;\nSelect w such that Parent*(_, _)");
    auto eval = pql::eval::Evaluator(pkb, query);

    auto result = eval.evaluate();
    std::unordered_set<std::string> result_s(result.begin(), result.end());

    REQUIRE(result_s.size() == 2);
    REQUIRE(result_s.count("8"));
    REQUIRE(result_s.count("13"));
}
