// test_parser.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"
#include <zpr.h>
#include <zst.h>


#include "simple/ast.h"
#include "simple/parser.h"
#include "simple/parser.cpp"

using namespace simple::parser;
using namespace simple::ast;

using zst::ErrFmt;

static void req(bool b)
{
    REQUIRE(b);
}

static bool check_expr(zst::str_view sv, zst::str_view expect, Result<Expr*> (*func)(ParserState*))
{
    auto ps = ParserState { sv };
    auto res = func(&ps);
    if(res.ok() && res.unwrap()->toString() == expect)
    {
        return true;
    }
    else if(!res.ok() && res.error() == expect)
    {
        return true;
    }
    else
    {
        zpr::fprintln(stderr, "Invalid test case\ngiven sv: {}, given expectation: {}, obtained res: {}", sv, expect,
            res.ok() ? res.unwrap()->toString() : res.error());
        return false;
    }
}

static bool check_prog(zst::str_view sv, zst::str_view expect)
{
    auto prog = parseProgram(sv);
    if(prog.ok() && prog.unwrap()->toProgFormat() == expect)
    {
        return true;
    }
    else if(!prog.ok() && prog.error() == expect)
    {
        return true;
    }
    else
    {
        zpr::fprintln(stderr, "Invalid test case\ngiven sv: {}, given expectation: {}, obtained res: {}", sv, expect,
            prog.ok() ? prog.unwrap()->toProgFormat() : prog.error());
        return false;
    }
}

// start from the bottom
TEST_CASE("Parse expr")
{
    SECTION("Valid cases")
    {
        req(check_expr("a * 2;", "(a * 2)", &parseExpr));
        req(check_expr("a = 2;", "a", &parseExpr));
        req(check_expr("a * 2;", "(a * 2)", &parseExpr));
    }
    SECTION("Invalid cases")
    {
        req(check_expr("*aa)", "invalid start of expression with '*'", &parseExpr));
    }
}


TEST_CASE("Parse primary")
{
    SECTION("Constant")
    {
        req(check_expr("22", "22", &parsePrimary));
    }
    SECTION("Var")
    {
        req(check_expr("aa", "aa", &parsePrimary));
    }
    SECTION("Expr in parenthesis")
    {
        req(check_expr("(aa)", "aa", &parsePrimary));
        req(check_expr("(aa", "expected ')'", &parsePrimary));
    }
    SECTION("Invalid expr")
    {
        req(check_expr("*aa)", "invalid start of expression with '*'", &parsePrimary));
    }
}

TEST_CASE("Parse rhs")
{
    req(true);
}

TEST_CASE("Parse conditional expr")
{
    SECTION("!")
    {
        std::string in = "a = 3;";
        auto ps = ParserState { in };
        parseExpr(&ps);
        req(true);
    }
    SECTION("(")
    {
        req(true);
    }
    SECTION("Expr")
    {
        req(true);
    }
}
TEST_CASE("Parse stmt list")
{
    req(true);
}
TEST_CASE("Parse stmt")
{
    req(true);
}
TEST_CASE("Parse if statement")
{
    req(true);
}
TEST_CASE("Parse while stmt")
{
    req(true);
}
TEST_CASE("Parse procedure")
{
    req(true);
}


TEST_CASE("Parse program")
{
    /*
    SECTION("Error: array")
    {
        std::string in = R"(
            procedure A {
                a = [];
            }
        )";
        std::string out = "invalid token '['";

        req(check_prog(in, out));
        req(prog.error().compare(out) == 0);
    }
    */
    SECTION("Error: pointer")
    {
        std::string in = R"(
            procedure A {
                *a = b;
            }
        )";
        std::string out = "unexpected token '*' at beginning of statement";
        req(check_prog(in, out));
    }
    SECTION("Invalid procedure syntax")
    {
        std::string in = R"(
            procedure A {

            }
        )";
        std::string out = "expected at least one statement between '{' and '}'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure {
                
            }
        )";
        std::string out2 = "expected identifier after 'procedure' keyword";
        req(check_prog(in2, out2));

        std::string in3 = R"(
            procedure A(a) {
                
            }
        )";
        std::string out3 = "expected '{'";
        req(check_prog(in3, out3));

        std::string in4 = R"(
            Procedure A {
                
            }
        )";
        std::string out4 = "expected 'procedure' to define a procedure (found 'Procedure')";
        req(check_prog(in4, out4));


        std::string in5 = R"(
            procedure A {
                a = 4;
                procedure B {
                    b = 5;
                }
            }
        )";
        std::string out5 = "expected '=' after identifier";
        req(check_prog(in5, out5));
    }
    SECTION("Error for invalid read syntax")
    {
        std::string in = R"(
            procedure A {
                read(A);
            }
        )";
        std::string out = "expected identifier after 'read'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                read a = 2;
            }
        )";
        std::string out2 = "expected semicolon after statement";
        req(check_prog(in2, out2));
    }


    SECTION("Error for invalid print syntax")
    {
        std::string in = R"(
            procedure A {
                print(A);
            }
        )";
        std::string out = "expected identifier after 'print'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                print a = 2;
            }
        )";
        std::string out2 = "expected semicolon after statement";
        req(check_prog(in2, out2));
    }
    SECTION("Error for invalid call syntax")
    {
        std::string in = R"(
            procedure A {
                call(A);
            }
        )";
        std::string out = "expected identifier after 'call'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                call a = 2;
            }
        )";
        std::string out2 = "expected semicolon after statement";
        req(check_prog(in2, out2));
    }
    SECTION("Error for invalid call syntax")
    {
        std::string in = R"(
            procedure A {
                call(A);
            }
        )";
        std::string out = "expected identifier after 'call'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                call a = 2;
            }
        )";
        std::string out2 = "expected semicolon after statement";
        req(check_prog(in2, out2));
    }
    /*
    SECTION("Error for invalid while syntax")
    {
        std::string in = R"(
            procedure A {
                while (a == 1 ) {
                }
            }
        )";
        std::string out = "expected at least one statement between '{' and '}'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                while ( a ) {
                }
            }
        )";
        std::string out2 = "invalid binary operator ')'";
        req(check_prog(in2, out2));

        std::string in3 = R"(
            procedure A {
                while ( a ) {
                    a = 3;
                }
            }
        )";
        std::string out3 = "invalid binary operator ')'";
        req(check_prog(in3, out3));

        std::string in4 = R"(
            procedure A {
                while a == 2 {
                }
            }
        )";
        std::string out4 = "expected '(' after 'while'";
        req(check_prog(in4, out4));

        std::string in5 = R"(
        procedure A {
            while (v + x * y + z * t < 10) {
                x = 0;
                if (x > 0)then {
                    a = a + 1;
                }else{a = 1;}
            }
        }
        )";
        std::string out5 = "expected '(' after 'while'";
        req(check_prog(in5, out5));

        std::string in6 = R"(
        procedure A {
            if (((((v + (x)) * y + z) * t) < 10) then {
                x = 0;
            } else {
                c = 0;
            }
        }
        )";
        std::string out6 = "expected ')' after '('";
        req(check_prog(in6, out6));

    }


    SECTION("Error for invalid if syntax")
    {
        std::string in = R"(
            procedure A {
                while ( a == 1 ) {
                    if (a == 1) then {
                    }
                }
            }
        )";
        std::string out = "expected at least one statement between '{' and '}'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                if ( a ) then {
                }
            }
        )";
        std::string out2 = "invalid binary operator ')'";
        req(check_prog(in2, out2));

        std::string in3 = R"(
            procedure A {
                if a == 2 {
                }
            }
        )";
        std::string out3 = "expected '(' after 'if'";
        req(check_prog(in3, out3));

        std::string in4 = R"(
            procedure A {
                if (a == 2) {
                }
            }
        )";
        std::string out4 = "expected 'then' after condition for 'if'";
        req(check_prog(in4, out4));

        std::string in5 = R"(
            procedure A {
                if (a == 2) then {
                    a = 5;
                }
            }
        )";
        std::string out5 = "'else' clause is mandatory";
        req(check_prog(in5, out5));


        std::string in6 = R"(
            procedure A {
                if (a == 2) then {
                    a = 5;
                } else {}
            }
        )";
        std::string out6 = "expected at least one statement between '{' and '}'";
        req(check_prog(in6, out6));

        std::string in7 = R"(
            procedure A {
                if (a == 2) then {
                    a = 5;
                } else if (a == 7) {
                    a = 6;
                }
            }
        )";
        std::string out7 = "expected '{'";
        req(check_prog(in7, out7));
    }
    */
    SECTION("Error for invalid assignment")
    {
        std::string in = R"(
            procedure A {
                a = b = 4;
            }
        )";
        std::string out = "expected semicolon after statement";
        req(check_prog(in, out));
        /*
        std::string in2 = R"(
            procedure A {
                a = 04;
            }
        )";
        std::string out2 = "multi-digit integer literal cannot start with 0";
        req(check_prog(in2, out2));
        req(prog2.error().compare(out2) == 0);*/
        /*
        std::string in3 = R"(
            procedure A {
                a = "fff";
            }
        )";
        std::string out3 = "multi-digit integer literal cannot start with 0";

        req(check_prog(in3, out3));
        zpr::fprintln(stdout, "begin{}end", prog3.error());
        req(prog3.error().compare(out3) == 0);
        */
    }

    SECTION("Error for invalid comparison")
    {
        std::string in = R"(
            procedure A {
                a => 4;
            }
        )";
        std::string out = "invalid start of expression with '>'";
        req(check_prog(in, out));

        std::string in2 = R"(
            procedure A {
                a == 4;
            }
        )";
        std::string out2 = "expected '=' after identifier";
        req(check_prog(in2, out2));
    }
    SECTION("Bad formatting")
    {
        std::string in = R"(
          procedure         a
                 {
                count = 2 *       3 + 4     * 5;
                a=10;
            }
        )";
        std::string out = R"(procedure a
{
    count = ((2 * 3) + (4 * 5));
    a = 10;
}
)";

        auto prog = parseProgram(in).unwrap();
        req(prog->toProgFormat().compare(out) == 0);
    }
}

TEST_CASE("Sample tests for parse program")
{
    SECTION("Assignments")
    {
        /*
                std::string in = R"(
                    procedure a
                    {
                        a1 = x + z;
                        a2 = x * z;
                        a3 = x + y + z;
                        a4 = x + z * 5;
                        a5 = z * 5 + x;
                        a6 = (x + z) * 5;
                        a7 = v + x * y + z * t;
                        while (v + x * y + z * t < 10) {
                            x = 0;
                        }
                    }
                    procedure b
                    {
                        if ( ((1==b) && (b == 1)) || (c == 1)) then {
                            a = 2;
                        } else {
                            a = 0;
                        }
                    }
                )";
                std::string out = R"(procedure a
        {
            a1 = (x + z);
            a2 = (x * z);
            a3 = ((x + y) + z);
            a4 = (x + (z * 5));
            a5 = ((z * 5) + x);
            a6 = ((x + z) * 5);
            a7 = ((v + (x * y)) + (z * t));
            while(((v + (x * y)) + (z * t)) < 10)
            {
                x = 0;
            }
        }
        procedure b
        {
            if(((1 == b) && (b == 1)) || (c == 1)) then
            {
                a = 2;
            }
            else
            {
                a = 0;
            }
        }
        )";
                req(check_prog(in, out));
         */
    }

    // 8 categories mentioned in tut. Combined some of the sections.
    SECTION("Standalone while loops") // at 3 different positions
    {
        std::string in = R"(procedure a
{
    while(a == b)
    {
        a = b;
    }
    c = 2;
    d = 3;
}
)";
        req(check_prog(in, in));


        std::string in2 = R"(procedure a
{
    c = 2;
    while(a == b)
    {
        a = b;
    }
    d = 3;
}
)";
        req(check_prog(in2, in2));

        std::string in3 = R"(procedure a
{
    c = 2;
    d = 3;
    while(a == b)
    {
        a = b;
    }
}
)";
        req(check_prog(in3, in3));
    }

    SECTION("Standalone if stmt and combination of if and while") // at 3 different positions
    {
        std::string in = R"(procedure a
{
    if(a == b) then
    {
        a = b;
    }
    else
    {
        a = b;
    }
    c = 2;
    d = 3;
}
)";
        req(check_prog(in, in));

        std::string in2 = R"(procedure a
{
    c = 2;
    d = 3;
    while(a == b)
    {
        a = b;
    }
    e = 3;
    if(a == b) then
    {
        a = b;
    }
    else
    {
        a = b;
    }
    d = 3;
}
)";
        req(check_prog(in2, in2));

        std::string in3 = R"(procedure a
{
    c = 2;
    d = 3;
    if(a == b) then
    {
        a = b;
    }
    else
    {
        a = b;
    }
}
procedure b
{
    c = 2;
    d = 3;
    while(a == b)
    {
        a = b;
    }
}
)";
        req(check_prog(in3, in3));
    }

    SECTION("2-level nesting of if and while and separate if/while inside a nest if/while")
    {
        std::string in = R"(procedure a
{
    while(a == b)
    {
        if(a == b) then
        {
            a = b;
        }
        else
        {
            a = b;
        }
        while(a == b)
        {
            a = b;
        }
    }
}
)";
        req(check_prog(in, in));


        std::string in2 = R"(procedure a
{
    while(a == b)
    {
        while(a == b)
        {
            a = b;
        }
    }
}
)";
        req(check_prog(in2, in2));

        std::string in3 = R"(procedure a
{
    if(a == b) then
    {
        if(a == b) then
        {
            a = b;
        }
        else
        {
            a = b;
        }
    }
    else
    {
        while(a == b)
        {
            a = b;
        }
    }
}
)";
        req(check_prog(in3, in3));

        std::string in4 = R"(procedure a
{
    if(a == b) then
    {
        while(a == b)
        {
            a = b;
        }
        if(a == b) then
        {
            a = b;
        }
        else
        {
            a = b;
        }
    }
    else
    {
        if(a == b) then
        {
            a = b;
        }
        else
        {
            a = b;
        }
    }
}
)";
        req(check_prog(in4, in4));
    }


    SECTION("Permutations of nesting 3 levels") // 8 permutations
    {
        std::string in = R"(procedure a
{
    if(a == b) then
    {
        if(a == b) then
        {
            if(a == b) then
            {
                a = b;
            }
            else
            {
                while(a == b)
                {
                    a = b;
                }
            }
        }
        else
        {
            a = b;
        }
    }
    else
    {
        while(a == b)
        {
            while(a == b)
            {
                a = b;
            }
            if(a == b) then
            {
                a = b;
            }
            else
            {
                a = b;
            }
        }
    }
    while(a == b)
    {
        while(a == b)
        {
            while(a == b)
            {
                a = b;
            }
        }
        if(a == b) then
        {
            a = b;
        }
        else
        {
            while(a == b)
            {
                a = b;
            }
        }
        if(a == b) then
        {
            a = b;
        }
        else
        {
            if(a == b) then
            {
                a = b;
            }
            else
            {
                a = b;
            }
        }
    }
}
)";
        req(check_prog(in, in));
    }

    SECTION(
        "Permutations of more than 4 levels of nesting and Permutations of separate if/while in both nested levels") // 8 perms
    {
        std::string in = R"(procedure a
{
    while(a == b)
    {
        if(a == b) then
        {
            while(a == b)
            {
                if(a == b) then
                {
                    while(a == b)
                    {
                        a = b;
                    }
                }
                else
                {
                    while(a == b)
                    {
                        a = b;
                    }
                }
            }
        }
        else
        {
            while(a == b)
            {
                while(a == b)
                {
                    if(a == b) then
                    {
                        if(a == b) then
                        {
                            while(a == b)
                            {
                                a = b;
                            }
                        }
                        else
                        {
                            while(a == b)
                            {
                                a = b;
                            }
                        }
                    }
                    else
                    {
                        while(a == b)
                        {
                            a = b;
                        }
                    }
                }
            }
        }
    }
}
)";
        req(check_prog(in, in));
    }
}
