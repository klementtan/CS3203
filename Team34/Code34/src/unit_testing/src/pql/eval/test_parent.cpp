// test_parent.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

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
    TEST_OK(test_program_1, "print p; Select p such that Parent(19, 22)", 6, 7, 8, 9, 25, 26);
}

TEST_CASE("Parent(StmtId, Decl)")
{
    TEST_OK(test_program_1, "assign a; Select a such that Parent(19, a)", 20, 21, 22);
}

TEST_CASE("Parent(StmtId, _)")
{
    TEST_OK(test_program_1, "print p; Select p such that Parent(19, _)", 6, 7, 8, 9, 25, 26);
}




TEST_CASE("Parent(Decl, StmtId)")
{
    TEST_OK(test_program_1, "if i; Select i such that Parent(i, 26)", 24);
}

TEST_CASE("Parent(Decl, Decl)")
{
    TEST_OK(test_program_1, "if i; assign a; Select i such that Parent(i, a)", 19);
}

TEST_CASE("Parent(Decl, _)")
{
    TEST_OK(test_program_1, "if i; Select i such that Parent(i, _)", 19, 24);
}



TEST_CASE("Parent(_, StmtId)")
{
    TEST_OK(test_program_1, "print p; Select p such that Parent(_, 21)", 6, 7, 8, 9, 25, 26);
}

TEST_CASE("Parent(_, Decl)")
{
    TEST_OK(test_program_1, "print p; Select p such that Parent(_, p)", 25, 26);
}

TEST_CASE("Parent(_, _)")
{
    TEST_EMPTY("procedure a { x = 1; }", "variable v; Select v such that Parent(_, _)");
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
    TEST_OK(test_program_2, "print p; Select p such that Parent*(1, 11)", 2, 4, 7, 9, 11, 14, 17, 20, 23);
}

TEST_CASE("Parent*(StmtId, Decl)")
{
    TEST_OK(test_program_2, "read r; Select r such that Parent*(8, r)", 19, 22);
}

TEST_CASE("Parent*(StmtId, _)")
{
    TEST_OK(test_program_2, "print p; Select p such that Parent*(8, _)", 2, 4, 7, 9, 11, 14, 17, 20, 23);
}

TEST_CASE("Parent*(Decl, StmtId)")
{
    TEST_OK(test_program_2, "if i; Select i such that Parent*(i, 20)", 1, 3, 10, 16);
}

TEST_CASE("Parent*(Decl, Decl)")
{
    TEST_OK(test_program_2, "if i; call c; Select i such that Parent*(i, c)", 1, 3);
}

TEST_CASE("Parent*(Decl, _)")
{
    TEST_OK(test_program_2, "while w; Select w such that Parent*(w, _)", 8, 13);
}

TEST_CASE("Parent*(_, StmtId)")
{
    TEST_OK(test_program_2, "while w; Select w such that Parent*(_, 2)", 8, 13);
}

TEST_CASE("Parent*(_, Decl)")
{
    TEST_OK(test_program_2, "if i; Select i such that Parent*(_, i)", 3, 10, 16);
}

TEST_CASE("Parent*(_, _)")
{
    TEST_OK(test_program_2, "while w; Select w such that Parent*(_, _)", 8, 13);
}

TEST_CASE("no parent")
{
    TEST_EMPTY("procedure a { x = 1; }", "variable v; Select v such that Parent(_, _)");
}
