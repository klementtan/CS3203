// pql/parser.h


#pragma once

#include <vector>
#include "ast.h"

namespace pql::parser
{
    enum class TokenType
    {
        EndOfFile = 0,

        LParen,
        RParen,
        LAngle,
        RAngle,

        Underscore,
        Semicolon,
        Comma,
        Dot,
        Equal,
        String,

        Identifier,
        Number,

        KW_Next,
        KW_Uses,
        KW_Calls,
        KW_Parent,
        KW_Follows,
        KW_Affects,
        KW_Modifies,

        KW_NextStar,
        KW_CallsStar,
        KW_ParentStar,
        KW_FollowsStar,
        KW_AffectsStar,

        KW_NextBip,
        KW_AffectsBip,

        KW_NextBipStar,
        KW_AffectsBipStar,

        KW_And,
        KW_With,
        KW_Select,
        KW_Pattern,
        KW_SuchThat,

        KW_Value,
        KW_StmtNum,
        KW_VarName,
        KW_ProcName,

        KW_ProgLine,
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

    const char* tokenTypeString(TokenType tt);

    Token peekNextKeywordToken(zst::str_view sv);
    Token getNextKeywordToken(zst::str_view& sv);

    Token getNextToken(zst::str_view& sv);
    Token peekNextToken(zst::str_view sv);
}

template <>
struct std::hash<pql::parser::Token>
{
    std::size_t operator()(const pql::parser::Token& t) const noexcept
    {
        return util::hash_combine(t.text.str(), t.type);
    }
};


namespace pql::parser
{
    std::unique_ptr<ast::Query> parsePQL(zst::str_view input);
}
