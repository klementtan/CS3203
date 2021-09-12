// pql/lexer.cpp

#include <zst.h>
#include "util.h"
#include "pql/parser/parser.h"
#include "pql/exception.h"

namespace pql::parser
{
    static void eat_whitespace(zst::str_view& sv)
    {
        while(!sv.empty() && (sv[0] == '\n' || sv[0] == '\r' || sv[0] == '\t' || sv[0] == ' '))
            sv.remove_prefix(1);
    }

    static bool is_letter(char c)
    {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
    }

    static bool is_digit(char c)
    {
        return '0' <= c && c <= '9';
    }

    Token peekNextOneToken(zst::str_view sv)
    {
        return getNextToken(sv);
    }

    std::vector<Token> peekNextTwoTokens(zst::str_view sv)
    {
        Token fst = getNextToken(sv);
        Token snd = getNextToken(sv);
        return { fst, snd };
    }

    // Extract characters till reaches '"'
    zst::str_view extractTillQuotes(zst::str_view& sv)
    {
        size_t num_chars = 0;
        while(!sv.empty() && sv[num_chars] != '"')
        {
            num_chars += 1;
        }
        zst::str_view ret = sv.take_prefix(num_chars);
        return ret;
    }

    Token getNextToken(zst::str_view& sv)
    {
        eat_whitespace(sv);

        if(sv.empty() || sv[0] == '\0')
        {
            return Token { "", TT::EndOfFile };
        }
        else if(is_letter(sv[0]))
        {
            size_t num_chars = 0;
            while(!sv.empty() && (is_digit(sv[num_chars]) || is_letter(sv[num_chars])))
                num_chars += 1;

            return Token { sv.take_prefix(num_chars), TT::Identifier };
        }
        else if(is_digit(sv[0]))
        {
            size_t num_chars = 0;
            while(!sv.empty() && is_digit(sv[num_chars]))
                num_chars += 1;

            return Token { sv.take_prefix(num_chars), TT::Number };
        }
        else
        {
            TokenType tt {};

            // clang-format is dumb and insists on destroying the formatting here.
            // clang-format off
            switch(sv[0])
            {
                case '(':   tt = TT::LParen; break;
                case ')':   tt = TT::RParen; break;
                case '_':   tt = TT::Underscore; break;
                case '"':   tt = TT::DoubleQuotes; break;
                case ';':   tt = TT::Semicolon; break;
                case '*':   tt = TT::Asterisk; break;
                case ',':   tt = TT::Comma; break;
                default:
                    throw pql::exception::PqlException("pql::parser","invalid token '{}'", sv[0]);
            }

            return Token { sv.take_prefix(1), tt };
        }
    }
}
