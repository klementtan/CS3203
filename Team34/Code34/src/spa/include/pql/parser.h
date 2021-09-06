// pql/parser.h


#pragma once

#include <zst.h>
#include <vector>
#include "pql/ast.h"

namespace pql::parser
{
    enum class TokenType
    {
        EndOfFile = 0,

        LParen,
        RParen,

        Asterisk,
        Undersocre,
        Semicolon,
        DoubleQuotes,
        Comma,

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

        bool operator==(const Token& other) const
        {
            return (this->type == other.type) && (this->text == other.text);
        }
    };

    using TT = TokenType;

    zst::str_view extractTillQuotes(zst::str_view& sv);
    Token getNextToken(zst::str_view& sv);
    Token peekNextOneToken(zst::str_view sv);
    std::vector<Token> peekNextTwoTokens(zst::str_view sv);
}

template <>
struct std::hash<pql::parser::Token>
{
    std::size_t operator()(const pql::parser::Token& t) const noexcept
    {
        return std::hash<std::string>()(t.text.str()) ^ std::hash<pql::parser::TokenType>()(t.type);
    }
};


namespace pql::parser
{
    pql::ast::Query* parsePQL(zst::str_view input);
}