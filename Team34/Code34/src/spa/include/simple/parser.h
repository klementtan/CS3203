// parser.h
// contains definitions for simple-parsing related code, namely the lexer & parser.

#pragma once

#include <cstdint>
#include <cstddef>

#include <string>
#include <vector>

#include <zpr.h>
#include <zst.h>

#include "ast.h"

// lexer stuff
namespace simple::parser
{
    enum class TokenType
    {
        EndOfFile = 0,

        LAngle,
        RAngle,
        LBrace,
        RBrace,
        LParen,
        RParen,

        Exclamation,
        LogicalAnd,
        LogicalOr,

        Equal, // =

        NotEqual,     // !=
        EqualsTo,     // ==
        GreaterEqual, // >=
        LessEqual,    // <=

        Plus,
        Minus,
        Asterisk,
        Slash,
        Percent,
        Semicolon,

        Identifier,
        Number,
    };

    struct Token
    {
        zst::str_view text;
        TokenType type;

        operator TokenType() const
        {
            return this->type;
        }
    };

    using TT = TokenType;

    void eat_whitespace(zst::str_view& sv);
    Token getNextToken(zst::str_view& sv);
    Token peekNextToken(zst::str_view sv);
}



// parser stuff
namespace simple::parser
{
    zst::Result<ast::Expr*, std::string> parseExpression(zst::str_view input);
    zst::Result<ast::Program*, std::string> parseProgram(zst::str_view input);
}
