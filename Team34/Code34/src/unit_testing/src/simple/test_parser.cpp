// test_parser.cpp

#include "catch.hpp"

#include <zpr.h>
#include <zst.h>

#include "simple/ast.h"
#include "simple/parser.h"

#include "runner.h"

TEST_CASE("whitespace handling")
{
    TEST_EXPR_OK("    abc  \n\n\n\n\n\n\t\t\t\t  ", "abc");
    TEST_EXPR_OK("1     +      2", "(1 + 2)");

    TEST_PROG_OK(R"(
          procedure         a
                 {
                count = 2 *       3 + 4     * 5;
                a=10;
            }
        )",
        "procedure a{count = ((2 * 3) + (4 * 5));a = 10;}");
}

TEST_CASE("expression parsing -- basic operators")
{
    SECTION("positive cases")
    {
        TEST_EXPR_OK("0+0", "(0 + 0)");
        TEST_EXPR_OK("1+2", "(1 + 2)");
        TEST_EXPR_OK("10*20", "(10 * 20)");
        TEST_EXPR_OK("20-cd", "(20 - cd)");
        TEST_EXPR_OK("ab/20", "(ab / 20)");
        TEST_EXPR_OK("69%69", "(69 % 69)");
        TEST_EXPR_OK("(a * a)", "(a * a)");
        TEST_EXPR_OK("(((((a * b)))))", "(a * b)");
        TEST_EXPR_OK("(((((((((a)))) * ((((b)))))))))", "(a * b)");
    }

    SECTION("negative cases")
    {
        // TODO: need a negative test case for this
        // TEST_EXPR_ERR("01", "multi-digit integer literal cannot start with 0");

        TEST_EXPR_ERR("-2", "invalid start of expression with '-'");
        TEST_EXPR_ERR("1+", "unexpected end of input");
        TEST_EXPR_ERR("a+2+", "unexpected end of input");
        TEST_EXPR_ERR("b+2*", "unexpected end of input");
        TEST_EXPR_ERR("(1+2", "expected ')' to match opening '('");

        TEST_EXPR_ERR("*(1+2", "invalid start of expression with '*'");

        TEST_EXPR_ERR("foo(", "unexpected token '(' after expression");

        // ensure that relational and conditionals cannot appear in normal expressions
        TEST_EXPR_ERR("1 < 2", "unexpected token '<' after expression");
        TEST_EXPR_ERR("1 + 3 < 2", "unexpected token '<' after expression");
        TEST_EXPR_ERR("1 + 3 && (2 > 0)", "unexpected token '&&' after expression");
        TEST_EXPR_ERR("1 + !3", "invalid start of expression with '!'");
    }
}

TEST_CASE("expression parsing -- associativity")
{
    TEST_EXPR_OK("1+2+3", "((1 + 2) + 3)");
    TEST_EXPR_OK("1-2-3", "((1 - 2) - 3)");

    TEST_EXPR_OK("1/2/3/4/5", "((((1 / 2) / 3) / 4) / 5)");
    TEST_EXPR_OK("1*2*3*4*5", "((((1 * 2) * 3) * 4) * 5)");
}

TEST_CASE("expression parsing -- precedence")
{
    TEST_EXPR_OK("1*2+3", "((1 * 2) + 3)");
    TEST_EXPR_OK("1+2*3", "(1 + (2 * 3))");
    TEST_EXPR_OK("1+2*3-4+5", "(((1 + (2 * 3)) - 4) + 5)");
    TEST_EXPR_OK("1+2*3-4+5/6*4", "(((1 + (2 * 3)) - 4) + ((5 / 6) * 4))");


    TEST_EXPR_OK("1*(2+3)", "(1 * (2 + 3))");
    TEST_EXPR_OK("(1+2)*3", "((1 + 2) * 3)");
    TEST_EXPR_OK("((((1+2))))*3", "((1 + 2) * 3)");

    TEST_EXPR_OK("(1+2)*(3-4)+5", "(((1 + 2) * (3 - 4)) + 5)");
    TEST_EXPR_OK("(1+2)*3-(4+5)/(6*4)", "(((1 + 2) * 3) - ((4 + 5) / (6 * 4)))");
}

// TODO: missing test cases for ensuring parser returns correct AST nodes
TEST_CASE("expression parsing -- ast types") { }

TEST_CASE("expression parsing -- relational") {
#define BEGIN "procedure foo{if("
#define END ")then{a = 1;}else{a = 0;}}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "1 > 2" END, BEGIN "1 > 2" END);
TEST_PROG_OK(BEGIN "1 < 2" END, BEGIN "1 < 2" END);
TEST_PROG_OK(BEGIN "1 == 2" END, BEGIN "1 == 2" END);
TEST_PROG_OK(BEGIN "1 != 2" END, BEGIN "1 != 2" END);
TEST_PROG_OK(BEGIN "1 <= 2" END, BEGIN "1 <= 2" END);
TEST_PROG_OK(BEGIN "1 >= 2" END, BEGIN "1 >= 2" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "1 < 2 < 3" END, "expected ')' after conditional, found '<' instead");
    TEST_PROG_ERR(BEGIN "1 < 2 > 3" END, "expected ')' after conditional, found '>' instead");
    TEST_PROG_ERR(BEGIN "1 == 2 != 3" END, "expected ')' after conditional, found '!=' instead");
    TEST_PROG_ERR(BEGIN "(z == y) < x" END, "relational operators cannot be chained");
    TEST_PROG_ERR(BEGIN "(z == y) < x < y" END, "relational operators cannot be chained");
    TEST_PROG_ERR(BEGIN "(z == y) < (x < y)" END, "relational operators cannot be chained");
}

#undef BEGIN
#undef END
}

TEST_CASE("expression parsing -- conditional") {
#define BEGIN "procedure foo{if("
#define END ")then{a = 1;}else{a = 0;}}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "!(x == 1)" END, BEGIN "!(x == 1)" END);
TEST_PROG_OK(BEGIN "!(!(x > 0))" END, BEGIN "!(!(x > 0))" END);
TEST_PROG_OK(BEGIN "!((((x < 69))))" END, BEGIN "!(x < 69)" END);

TEST_PROG_OK(BEGIN "v + x * y + z * t < 10" END, BEGIN "((v + (x * y)) + (z * t)) < 10" END);

TEST_PROG_OK(BEGIN "((v + (x * y)) / (z * t)) < 10" END, BEGIN "((v + (x * y)) / (z * t)) < 10" END);
TEST_PROG_OK(BEGIN "((v + (x * y)) + (z * t) - 3) < 10" END, BEGIN "(((v + (x * y)) + (z * t)) - 3) < 10" END);
TEST_PROG_OK(BEGIN "(((v + (x * y)) + 1) - 4) < 10" END, BEGIN "(((v + (x * y)) + 1) - 4) < 10" END);

TEST_PROG_OK(BEGIN "10 > 4 + (((v + (x * y)) + 1) - 4)" END, BEGIN "10 > (4 + (((v + (x * y)) + 1) - 4))" END);

TEST_PROG_OK(BEGIN "(((1 + 2))) / 4 > ((10))" END, BEGIN "((1 + 2) / 4) > 10" END);

TEST_PROG_OK(BEGIN "(1 == 2) && (!(2 == 1))" END, BEGIN "(1 == 2) && (!(2 == 1))" END);
TEST_PROG_OK(BEGIN "(x != y) || (p >= q)" END, BEGIN "(x != y) || (p >= q)" END);
TEST_PROG_OK(BEGIN "((1 == 2) && (2 == 1)) || (2 == 2)" END, BEGIN "((1 == 2) && (2 == 1)) || (2 == 2)" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "x" END, "invalid relational operator ')'");

    TEST_PROG_ERR(BEGIN "1 == 2 && (2 == 1) && (x <= y)" END, "expected ')' after conditional, found '&&' instead");
    TEST_PROG_ERR(BEGIN "(1 == 2) && (2 == 1) || (x <= y)" END, "expected ')' after conditional, found '||' instead");

    TEST_PROG_ERR(BEGIN "(1 == 2) &&" END, "expected '(' after '&&', found ')' instead");

    TEST_PROG_ERR(BEGIN "!x" END, "expected '(' after '!', found 'x' instead");
    TEST_PROG_ERR(BEGIN "!!(x)" END, "expected '(' after '!', found '!' instead");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- basic") {
#define BEGIN "procedure foo{"
#define END "}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "print x;" END, BEGIN "print x;" END);
TEST_PROG_OK(BEGIN "print x    ;" END, BEGIN "print x;" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "print x;;;;" END, "unexpected token ';' at beginning of statement");
    TEST_PROG_ERR(BEGIN ";" END, "unexpected token ';' at beginning of statement");
    TEST_PROG_ERR(BEGIN "*x = 3;" END, "unexpected token '*' at beginning of statement");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- print/read/call") {
#define BEGIN "procedure foo{"
#define END "}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "print x;" END, BEGIN "print x;" END);
TEST_PROG_OK(BEGIN "read x;" END, BEGIN "read x;" END);
TEST_PROG_OK(BEGIN "call x;" END, BEGIN "call x;" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "print 1;" END, "expected identifier after 'print'");
    TEST_PROG_ERR(BEGIN "read 1;" END, "expected identifier after 'read'");
    TEST_PROG_ERR(BEGIN "call 1;" END, "expected identifier after 'call'");

    TEST_PROG_ERR(BEGIN "print (1);" END, "expected identifier after 'print'");
    TEST_PROG_ERR(BEGIN "read (1);" END, "expected identifier after 'read'");
    TEST_PROG_ERR(BEGIN "call (1);" END, "expected identifier after 'call'");

    TEST_PROG_ERR(BEGIN "print x + y;" END, "expected semicolon after statement, found '+' instead");
    TEST_PROG_ERR(BEGIN "read x = 3;" END, "expected semicolon after statement, found '=' instead");
    TEST_PROG_ERR(BEGIN "call x();" END, "expected semicolon after statement, found '(' instead");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- assignments") {
#define BEGIN "procedure foo{"
#define END "}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "x = 1;" END, BEGIN "x = 1;" END);
TEST_PROG_OK(BEGIN "x = asdf;" END, BEGIN "x = asdf;" END);
TEST_PROG_OK(BEGIN "x = 1 + 2;" END, BEGIN "x = (1 + 2);" END);
TEST_PROG_OK(BEGIN "x = 1 + 2 * 3 - 4 / 7 % 69;" END, BEGIN "x = ((1 + (2 * 3)) - ((4 / 7) % 69));" END);
TEST_PROG_OK(BEGIN "x = (((((1 + 2 * 3 - 4 / 7 % 69)))));" END, BEGIN "x = ((1 + (2 * 3)) - ((4 / 7) % 69));" END);
TEST_PROG_OK(BEGIN "x = (((((p + q) * r - s / t % 69))));" END, BEGIN "x = (((p + q) * r) - ((s / t) % 69));" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "x = ;" END, "invalid start of expression with ';'");
    TEST_PROG_ERR(BEGIN "x = 1 > 2;" END, "expected semicolon after statement, found '>' instead");
    TEST_PROG_ERR(BEGIN "x = 1 = 2;" END, "expected semicolon after statement, found '=' instead");
    TEST_PROG_ERR(BEGIN "x = print 69;" END, "expected semicolon after statement, found '69' instead");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- blocks") {
#define BEGIN "procedure foo{while((2 + 2) == 5){"
#define END "}}"

    SECTION("positive cases") { TEST_PROG_OK(BEGIN "a=1;" END, BEGIN "a = 1;" END);
TEST_PROG_OK(BEGIN "a=1;b=2;c=3;" END, BEGIN "a = 1;b = 2;c = 3;" END);

TEST_PROG_OK(BEGIN "while(x == 1){b=2;}c=3;" END, BEGIN "while(x == 1){b = 2;}c = 3;" END);
TEST_PROG_OK(BEGIN "a=1;while(x == 1){b=2;}c=3;" END, BEGIN "a = 1;while(x == 1){b = 2;}c = 3;" END);
TEST_PROG_OK(BEGIN "a=1;while(x == 1){b=2;}" END, BEGIN "a = 1;while(x == 1){b = 2;}" END);


TEST_PROG_OK(BEGIN "if(x == 1)then{b=2;}else{x=0;}c=3;" END, BEGIN "if(x == 1)then{b = 2;}else{x = 0;}c = 3;" END);
TEST_PROG_OK(
    BEGIN "a=1;if(x == 1)then{b=2;}else{x=0;}c=3;" END, BEGIN "a = 1;if(x == 1)then{b = 2;}else{x = 0;}c = 3;" END);
TEST_PROG_OK(BEGIN "a=1;if(x == 1)then{b=2;}else{x=0;}" END, BEGIN "a = 1;if(x == 1)then{b = 2;}else{x = 0;}" END);

TEST_PROG_OK(BEGIN "while(x == 1){b=2;}c=3;if(x==0)then{c=0;}else{y=0;}while(z==0){p=0;}" END,
    BEGIN "while(x == 1){b = 2;}c = 3;if(x == 0)then{c = 0;}else{y = 0;}while(z == 0){p = 0;}" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "{ }" END, "unexpected token '{' at beginning of statement");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- while loop") {
#define BEGIN "procedure foo{"
#define END "}"

    // the conditional expression tests already covered the actual conditionals, so there's no need to repeat them here.
    SECTION("positive cases") { TEST_PROG_OK(BEGIN "while(1 < 2) { a = 1; }" END, BEGIN "while(1 < 2){a = 1;}" END);
}

SECTION("negative cases")
{
    TEST_PROG_ERR(BEGIN "while(x) { }" END, "invalid relational operator ')'");
    TEST_PROG_ERR(BEGIN "while(x - 1) { }" END, "invalid relational operator ')'");

    TEST_PROG_ERR(BEGIN "while() { }" END, "invalid start of expression with ')'");
    TEST_PROG_ERR(BEGIN "while(2+2==5)" END, "expected '{'");

    TEST_PROG_ERR(BEGIN "while 3 > 1 { a = 1; }" END, "expected '(' after 'while'");

    TEST_PROG_ERR(BEGIN "while(2 + 2 == 5) { }" END, "expected at least one statement between '{' and '}'");
}

#undef BEGIN
#undef END
}

TEST_CASE("statement parsing -- if statement")
{
#define BEGIN "procedure foo{"
#define END "}"

    // the conditional expression tests already covered the actual conditionals, so there's no need to repeat them here.
    SECTION("positive cases")
    {
        TEST_PROG_OK(BEGIN "if(1 < 2)then{a = 1;} else { b=0; }" END, BEGIN "if(1 < 2)then{a = 1;}else{b = 0;}" END);
    }

    SECTION("negative cases")
    {
        TEST_PROG_ERR(BEGIN "if 3 > 1 then { a = 1; } else { b = 1; }" END, "expected '(' after 'if'");

        TEST_PROG_ERR(BEGIN "if(2+2==5) { }" END, "expected 'then' after condition for 'if'");
        TEST_PROG_ERR(BEGIN "if(2+2==5)then" END, "expected '{'");
        TEST_PROG_ERR(BEGIN "if(2+2==5)then{a=0;}" END, "'else' clause is mandatory");
        TEST_PROG_ERR(BEGIN "if(2+2==5)then{a=0;}else" END, "expected '{'");
        TEST_PROG_ERR(BEGIN "if(2+2==5)then{a=0;}else{}" END, "expected at least one statement between '{' and '}'");
        TEST_PROG_ERR(BEGIN "if(2+2==5)then{a=0;}else if{}" END, "expected '{'");
    }

#undef BEGIN
#undef END
}


#define PROC "procedure foo{"

#define END "}"
#define X "x = 0;"
#define Y "print x;"
#define WHILE "while(x == 0){"
#define IF "if(x == 0)then{"
#define ELSE "}else{"

TEST_CASE("statement parsing -- if/while nesting (1 level, adjacent/standalone)")
{
    auto a = PROC X X X IF X ELSE X END X X X IF X ELSE X END X X X IF X ELSE X END X X END;
    auto b = PROC X X X WHILE X END X X X IF X ELSE X END X X X WHILE X END X X END;
    auto c = PROC X X X IF X ELSE X END X X X WHILE X END X X X IF X ELSE X END X X END;

    TEST_PROG_OK(a, a);
    TEST_PROG_OK(b, b);
    TEST_PROG_OK(c, c);
}

TEST_CASE("statement parsing -- if/while nesting (2 levels)")
{
    auto a = PROC WHILE WHILE X X END END END;
    auto b = PROC WHILE X WHILE X X END X END END;
    auto c = PROC X WHILE X WHILE X X END WHILE X END X END X END;
    auto d = PROC X WHILE X IF X ELSE X X X END WHILE X END X END X END;
    auto e = PROC IF IF X ELSE X END ELSE WHILE X END WHILE X END END WHILE X END END;

    TEST_PROG_OK(a, a);
    TEST_PROG_OK(b, b);
    TEST_PROG_OK(c, c);
    TEST_PROG_OK(d, d);
    TEST_PROG_OK(e, e);
}

TEST_CASE("statement parsing -- if/while nesting (3 levels)")
{
    auto a = PROC WHILE X WHILE X WHILE X X END X END X END X END;
    auto b =
        PROC IF IF IF X ELSE X END ELSE IF X ELSE X END END ELSE IF IF X ELSE X END ELSE IF X ELSE X END END END END;
    auto c = PROC WHILE IF WHILE X END ELSE IF X ELSE X END END END END;

    TEST_PROG_OK(a, a);
    TEST_PROG_OK(b, b);
    TEST_PROG_OK(c, c);
}

TEST_CASE("statement parsing -- if/while nesting (many levels)")
{
    auto a = PROC WHILE X WHILE X WHILE WHILE WHILE WHILE WHILE WHILE X END END END END END END X END X END X END;
    auto b = PROC IF IF IF IF X ELSE Y END ELSE IF IF X ELSE Y END ELSE Y END END ELSE IF IF X ELSE Y END ELSE Y END END
        ELSE IF IF IF X ELSE Y END ELSE IF IF X ELSE Y END ELSE IF X ELSE IF X ELSE Y END END END END ELSE IF IF X ELSE
            Y END ELSE IF X ELSE Y END END END END END;

    TEST_PROG_OK(a, a);
    TEST_PROG_OK(b, b);
}

#undef PROC
#define PROC(x) "procedure " x "{"

TEST_CASE("procedure parsing")
{
    SECTION("positive cases")
    {
        auto a = PROC("a1b2") X Y X Y END PROC("b2c3") Y Y Y END PROC("c3d4") X END;

        // procedures with "keyword" names
        auto b = PROC("if") X END;
        auto c = PROC("read") X END;
        auto d = PROC("call") X END;
        auto e = PROC("then") X END;
        auto f = PROC("else") X END;
        auto g = PROC("print") X END;
        auto h = PROC("while") X END;
        auto i = PROC("procedure") X END;

        TEST_PROG_OK(a, a);
        TEST_PROG_OK(b, b);
        TEST_PROG_OK(c, c);
        TEST_PROG_OK(d, d);
        TEST_PROG_OK(e, e);
        TEST_PROG_OK(f, f);
        TEST_PROG_OK(g, g);
        TEST_PROG_OK(h, h);
        TEST_PROG_OK(i, i);
    }

    SECTION("negative cases")
    {
        TEST_PROG_ERR("procedure { }", "expected identifier after 'procedure'");
        TEST_PROG_ERR("PROC { }", "expected 'procedure' to define a procedure (found 'PROC')");
        TEST_PROG_ERR("1 { }", "expected 'procedure' to define a procedure (found '1')");

        TEST_PROG_ERR("procedure 1 { }", "expected identifier after 'procedure'");
        TEST_PROG_ERR("procedure foo(x) { }", "expected '{'");

        TEST_PROG_ERR("procedure foo { procedure bar { a = 0; } b = 0; }", "expected '=' after identifier");
    }
}

#undef PROC
#undef END
#undef X
#undef WHILE
#undef IF
#undef ELSE
