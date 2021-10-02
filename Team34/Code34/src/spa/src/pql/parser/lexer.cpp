// pql/lexer.cpp


#include "util.h"
#include "exceptions.h"
#include "pql/parser/parser.h"

namespace pql::parser
{
    int eatWhitespace(zst::str_view& sv)
    {
        int count = 0;
        while(!sv.empty() && (sv[0] == '\n' || sv[0] == '\r' || sv[0] == '\t' || sv[0] == ' '))
        {
            count++;
            sv.remove_prefix(1);
        }
        return count;
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

    Token getNextKeywordToken(zst::str_view& sv)
    {
        eatWhitespace(sv);

        Token ret {};

        if(auto kw = "Next"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Next };

        else if(auto kw = "Uses"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Uses };

        else if(auto kw = "Calls"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Calls };

        else if(auto kw = "Parent"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Parent };

        else if(auto kw = "Follows"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Follows };

        else if(auto kw = "Affects"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Affects };

        else if(auto kw = "Modifies"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Modifies };

        else if(auto kw = "Next*"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_NextStar };

        else if(auto kw = "Calls*"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_CallsStar };

        else if(auto kw = "Parent*"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_ParentStar };

        else if(auto kw = "Follows*"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_FollowsStar };

        else if(auto kw = "Affects*"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_AffectsStar };

        else if(auto kw = "and"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_And };

        else if(auto kw = "with"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_With };

        else if(auto kw = "Select"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Select };

        else if(auto kw = "pattern"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_Pattern };

        else if(auto kw = "such that"; sv.find(kw) == 0)
            ret = Token { sv.take_prefix(strlen(kw)), TT::KW_SuchThat };

        else
        {
            throw util::PqlSyntaxException(
                "pql::parser", "expected a keyword (relation, 'such that', etc.), found '{}' instead", sv.take(10));
        }

        if(!sv.empty() && is_letter(sv[0]))
            throw util::PqlSyntaxException("pql::parser", "unexpected '{}' after keyword", sv[0]);

        return ret;
    }

    Token getNextToken(zst::str_view& sv)
    {
        eatWhitespace(sv);

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
        else if(sv[0] == '"')
        {
            sv.remove_prefix(1);

            size_t num_chars = 0;
            while(!sv.empty() && sv[num_chars] != '"')
                num_chars += 1;

            if(sv.empty())
                throw util::PqlSyntaxException("pql::parser", "unterminated expression string (expected '\"')");

            auto str = sv.take_prefix(num_chars);
            assert(sv[0] == '"');
            sv.remove_prefix(1);

            return { str, TT::String };
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
                case '<':   tt = TT::LAngle; break;
                case '>':   tt = TT::RAngle; break;
                case '_':   tt = TT::Underscore; break;
                case ';':   tt = TT::Semicolon; break;
                case '*':   tt = TT::Asterisk; break;
                case ',':   tt = TT::Comma; break;
                case '.':   tt = TT::Dot; break;
                case '#':   tt = TT::HashTag; break;
                default:
                    throw util::PqlSyntaxException("pql::parser","invalid token '{}'", sv[0]);
            }

            return Token { sv.take_prefix(1), tt };
        }
    }
}
