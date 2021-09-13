// test_uses.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pkb.h"
#include "simple/parser.h"
#include "pql/parser/parser.h"
#include "pql/eval/evaluator.h"

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

    TEST_OK(pkb, R"(variable v; Select v such that Uses("main", "flag"))", "x", "y", "count", "cenX", "cenY", "flag",
        "normSq");
}

TEST_CASE("UsesP(Decl, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(procedure p; Select p such that Uses(p, "cenX"))", "main", "printResults", "computeCentroid");
}

TEST_CASE("UsesP(Name, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(
        pkb, R"(variable v; Select v such that Uses("main", v))", "x", "y", "count", "cenX", "cenY", "flag", "normSq");
}

TEST_CASE("UsesP(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(
        pkb, R"(procedure p; variable v; Select p such that Uses(p, v))", "main", "printResults", "computeCentroid");
}

TEST_CASE("UsesP(Name, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(procedure p; Select p such that Uses("printResults", _))", "main", "readPoint", "printResults",
        "computeCentroid");
}

TEST_CASE("UsesP(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(procedure p; Select p such that Uses(p, _))", "main", "printResults", "computeCentroid");
}




TEST_CASE("UsesS(StmtId, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(print p; Select p such that Uses(2, "x"))", 6, 7, 8, 9);
}

TEST_CASE("UsesS(StmtId, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(variable v; Select v such that Uses(14, v))", "x", "y", "cenX", "cenY", "count");
}

TEST_CASE("UsesS(StmtId, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_EMPTY(pkb, R"(variable v; Select v such that Uses(18, _))");
}

TEST_CASE("UsesS(Decl, Name)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(assign a; Select a such that Uses(a, "cenX"))", 16, 21, 23);
}

TEST_CASE("UsesS(Decl, Decl)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(if i; variable v; Select v such that Uses(i, v))", "cenX", "cenY", "count");
}

TEST_CASE("UsesS(Decl, _)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_OK(pkb, R"(assign a; Select a such that Uses(a, _))", 15, 16, 17, 21, 22, 23);
}

TEST_CASE("Uses(_, *)")
{
    auto prog = simple::parser::parseProgram(test_program);
    auto pkb = pkb::processProgram(prog.unwrap()).unwrap();

    TEST_ERR(pkb, R"(variable v; Select v such that Uses(_, v))", "first argument of Uses cannot be '_'");
}
