// lexer.cpp

#include <zst.h>
#include <zpr.h>

#include "frontend.h"

namespace frontend
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

    Token peekNextToken(zst::str_view sv)
    {
        return getNextToken(sv);
    }

    Token getNextToken(zst::str_view& sv)
    {
        eat_whitespace(sv);

        if(sv.empty() || sv[0] == '\0')
        {
            return Token { "", TT::EndOfFile };
        }
        else if(sv.find(">=") == 0)
        {
            return Token { sv.take_prefix(2), TT::GreaterEqual };
        }
        else if(sv.find("<=") == 0)
        {
            return Token { sv.take_prefix(2), TT::LessEqual };
        }
        else if(sv.find("!=") == 0)
        {
            return Token { sv.take_prefix(2), TT::NotEqual };
        }
        else if(sv.find("==") == 0)
        {
            return Token { sv.take_prefix(2), TT::EqualsTo };
        }
        else if(sv.find("&&") == 0)
        {
            return Token { sv.take_prefix(2), TT::LogicalAnd };
        }
        else if(sv.find("||") == 0)
        {
            return Token { sv.take_prefix(2), TT::LogicalOr };
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
                case '<':   tt = TT::LAngle; break;
                case '>':   tt = TT::RAngle; break;
                case '{':   tt = TT::LBrace; break;
                case '}':   tt = TT::RBrace; break;
                case '(':   tt = TT::LParen; break;
                case ')':   tt = TT::RParen; break;
                case '+':   tt = TT::Plus; break;
                case '-':   tt = TT::Minus; break;
                case '*':   tt = TT::Asterisk; break;
                case '/':   tt = TT::Slash; break;
                case '%':   tt = TT::Percent; break;
                case '=':   tt = TT::Equal; break;
                case ';':   tt = TT::Semicolon; break;
                case '!':   tt = TT::Exclamation; break;
                default:
                    zpr::fprintln(stderr, "invalid token '{}'", sv[0]);
                    abort();
            }

            return Token { sv.take_prefix(1), tt };
        }
    }
}
