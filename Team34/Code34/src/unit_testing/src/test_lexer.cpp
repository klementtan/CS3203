// test_lexer.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include <zpr.h>
#include <zst.h>
#include "simple/parser.h"

using namespace simple::parser;

static void require(bool b)
{
    REQUIRE(b);
}

template <typename... Tokens>
static void check_tokens(zst::str_view sv, Tokens&&... tokens)
{
    auto check_token = [&sv](auto&& tok) {
        require(getNextToken(sv) == tok);
    };

    (check_token(static_cast<Tokens&&>(tokens)), ...);
    require(getNextToken(sv) == TT::EndOfFile);
}

TEST_CASE("tmp")
{
    parseProgram("procedure foo { while((v + x * y + z * t) < 10) { } }").unwrap();
}

// first, the single-character tokens
TEST_CASE("basic tokens")
{
    check_tokens("", TT::EndOfFile);
    check_tokens("<", TT::LAngle);
    check_tokens(">", TT::RAngle);
    check_tokens("{", TT::LBrace);
    check_tokens("}", TT::RBrace);
    check_tokens("(", TT::LParen);
    check_tokens(")", TT::RParen);
    check_tokens("!", TT::Exclamation);
    check_tokens("&&", TT::LogicalAnd);
    check_tokens("||", TT::LogicalOr);
    check_tokens("=", TT::Equal);
    check_tokens("!=", TT::NotEqual);
    check_tokens("==", TT::EqualsTo);
    check_tokens(">=", TT::GreaterEqual);
    check_tokens("<=", TT::LessEqual);
    check_tokens("+", TT::Plus);
    check_tokens("-", TT::Minus);
    check_tokens("*", TT::Asterisk);
    check_tokens("/", TT::Slash);
    check_tokens("%", TT::Percent);
    check_tokens(";", TT::Semicolon);
}

// then, some identifiers. simple grammar has no underscores.
TEST_CASE("basic identifiers")
{
    check_tokens("a12345bcdef", TT::Identifier);
    check_tokens("abcdef", TT::Identifier);
    check_tokens("a12345", TT::Identifier);
    check_tokens("a", TT::Identifier);
}

// then, numbers. these are only integers, and they are all positive
TEST_CASE("basic numbers")
{
    check_tokens("0", TT::Number);
    check_tokens("12345", TT::Number);
    check_tokens("9999999999", TT::Number);
}

// now, check multiple tokens.
TEST_CASE("basic tokens 2")
{
    check_tokens("= =", TT::Equal, TT::Equal);
    check_tokens("< =", TT::LAngle, TT::Equal);
    check_tokens("> =", TT::RAngle, TT::Equal);
    check_tokens("! =", TT::Exclamation, TT::Equal);
}

TEST_CASE("token sequencing")
{
    check_tokens("100aaa000bbb", TT::Number, TT::Identifier);
    check_tokens("foobar(123 456 abcdef);", TT::Identifier, TT::LParen, TT::Number, TT::Number, TT::Identifier,
        TT::RParen, TT::Semicolon);
}
