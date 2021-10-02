// test_lexer.cpp
//
// Unit test for pql/lexer.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "pql/parser/parser.h"


TEST_CASE("expr string")
{
    zst::str_view in = "\"1*2+73/414*34\")foobar";
    zst::str_view ret = pql::parser::getNextToken(in).text;
    REQUIRE(in == ")foobar");
    REQUIRE(ret == "1*2+73/414*34");
}
