// frontend.h
// contains definitions for frontend related code, namely the lexer & parser.

#pragma once

#include <cstdint>
#include <cstddef>

#include <string>
#include <vector>

#include <zst.h>

#include "ast.h"

// misc stuff
namespace frontend
{
    std::string readEntireFile(const char* path);
}


// lexer stuff
namespace frontend
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

    Token getNextToken(zst::str_view& sv);
    Token peekNextToken(zst::str_view sv);
}



// parser stuff
namespace frontend
{
    ast::Program* parseProgram(zst::str_view input);
}
