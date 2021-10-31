// test_uses.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

constexpr const auto prog_1 = "procedure main {\n"
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

constexpr const auto prog_2 = R"(
procedure foo {
    read x;
    read y;
    z = 10;
}
procedure bar {
    print a;
    print b;
    while(c == 0) {
        print d;
    }
}
)";

constexpr const auto prog_3 = R"(
procedure foo {
    read x;
    read y;
}
procedure bar {
    read a;
    read b;
}
)";

TEST_CASE("UsesP(Name, Name)")
{
    TEST_OK(prog_1, R"(variable v; Select v such that Uses("main", "flag"))", "x", "y", "count", "cenX", "cenY", "flag",
        "normSq");
}

TEST_CASE("UsesP(Decl, Name)")
{
    TEST_OK(prog_1, R"(procedure p; Select p such that Uses(p, "cenX"))", "main", "printResults", "computeCentroid");
}

TEST_CASE("UsesP(Name, Decl)")
{
    TEST_OK(prog_1, R"(variable v; Select v such that Uses("main", v))", "x", "y", "count", "cenX", "cenY", "flag",
        "normSq");
}

TEST_CASE("UsesP(Decl, Decl)")
{
    TEST_OK(
        prog_1, R"(procedure p; variable v; Select p such that Uses(p, v))", "main", "printResults", "computeCentroid");

    TEST_OK(prog_2, "procedure p; variable v; Select p such that Uses(p, v)", "bar");
    TEST_OK(prog_2, "procedure p; variable v; Select v such that Uses(p, v)", "a", "b", "c", "d");

    TEST_EMPTY(prog_3, "procedure p; variable v; Select p such that Uses(p, v)");
    TEST_EMPTY(prog_3, "procedure p; variable v; Select v such that Uses(p, v)");
}

TEST_CASE("UsesP(Name, _)")
{
    TEST_OK(prog_1, R"(procedure p; Select p such that Uses("printResults", _))", "main", "readPoint", "printResults",
        "computeCentroid");
}

TEST_CASE("UsesP(Decl, _)")
{
    TEST_OK(prog_1, R"(procedure p; Select p such that Uses(p, _))", "main", "printResults", "computeCentroid");
}




TEST_CASE("UsesS(StmtId, Name)")
{
    TEST_OK(prog_1, R"(print p; Select p such that Uses(2, "x"))", 6, 7, 8, 9);
}

TEST_CASE("UsesS(StmtId, Decl)")
{
    TEST_OK(prog_1, R"(variable v; Select v such that Uses(14, v))", "x", "y", "cenX", "cenY", "count");
}

TEST_CASE("UsesS(StmtId, _)")
{
    TEST_EMPTY(prog_1, R"(variable v; Select v such that Uses(18, _))");
}

TEST_CASE("UsesS(Decl, Name)")
{
    TEST_OK(prog_1, R"(assign a; Select a such that Uses(a, "cenX"))", 16, 21, 23);

    // test spaces (see PR #203)
    TEST_OK(prog_1, R"(assign a; Select a such that Uses(a, "   cenX       "))", 16, 21, 23);
}

TEST_CASE("UsesS(Decl, Decl)")
{
    TEST_OK(prog_1, R"(if i; variable v; Select v such that Uses(i, v))", "cenX", "cenY", "count");

    TEST_EMPTY(prog_2, "read r; variable v; Select r such that Uses(r, v)");
    TEST_EMPTY(prog_2, "read r; variable v; Select v such that Uses(r, v)");

    TEST_EMPTY(prog_3, "read r; variable v; Select r such that Uses(r, v)");
    TEST_EMPTY(prog_3, "read r; variable v; Select v such that Uses(r, v)");
}

TEST_CASE("UsesS(Decl, _)")
{
    TEST_OK(prog_1, R"(assign a; Select a such that Uses(a, _))", 15, 16, 17, 21, 22, 23);
}

TEST_CASE("Uses(_, *)")
{
    TEST_EMPTY(prog_1, R"(variable v; Select v such that Uses(_, v))");
}
