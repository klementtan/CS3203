// parser.cpp

#include <zpr.h>

#include <cassert>

#include "util.h"
#include "exceptions.h"
#include "simple/ast.h"
#include "simple/parser.h"

namespace simple::parser
{
    using namespace simple::ast;

    // this is just a convenience wrapper, nothing special.
    struct ParserState
    {
        zst::str_view stream;

        Token next()
        {
            return getNextToken(this->stream);
        }

        Token peek() const
        {
            return peekNextToken(this->stream);
        }
    };

    constexpr auto KW_Procedure = "procedure";
    constexpr auto KW_Read = "read";
    constexpr auto KW_Print = "print";
    constexpr auto KW_If = "if";
    constexpr auto KW_Then = "then";
    constexpr auto KW_Else = "else";
    constexpr auto KW_Call = "call";
    constexpr auto KW_While = "while";

    static int get_precedence(TokenType tt)
    {
        switch(tt)
        {
            case TT::Plus:
            case TT::Minus:
                return 1;

            case TT::Asterisk:
            case TT::Slash:
            case TT::Percent:
                return 2;

            default:
                return -1;
        }
    }

    static Expr* parseExpr(ParserState* ps);
    static Expr* parsePrimary(ParserState* ps)
    {
        if(ps->peek() == TT::LParen)
        {
            ps->next();
            auto ret = parseExpr(ps);
            if(ps->next() != TT::RParen)
                throw util::ParseException("simple::parser", "expected ')' to match opening '('");

            return ret;
        }
        else if(ps->peek() == TT::Number)
        {
            auto num = ps->next().text;
            if(num.size() > 1 && num[0] == '0')
                throw util::ParseException("simple::parser", "multi-digit integer literal cannot start with 0");

            auto constant = new Constant();
            for(char c : num)
                constant->value = 10 * constant->value + (c - '0');

            return constant;
        }
        else if(ps->peek() == TT::Identifier)
        {
            auto vr = new VarRef();
            vr->name = ps->next().text.str();

            return vr;
        }
        else if(ps->peek() == TT::EndOfFile)
        {
            throw util::ParseException("simple::parser", "unexpected end of input");
        }
        else
        {
            throw util::ParseException("simple::parser", "invalid start of expression with '{}'", ps->peek().text);
        }
    }

    static Expr* parseRhs(ParserState* ps, Expr* lhs, int priority)
    {
        if(priority == -1)
            return lhs;

        // everything here is left-associative, so there is no problem.
        while(true)
        {
            auto prec = get_precedence(ps->peek());
            if(prec < priority)
                return lhs;

            auto op = ps->next().text;
            auto rhs = parsePrimary(ps);
            assert(rhs);

            auto next = get_precedence(ps->peek());
            if(next > prec)
            {
                rhs = parseRhs(ps, rhs, prec + 1);
                assert(rhs);
            }

            auto binop = new BinaryOp();
            binop->lhs = lhs;
            binop->rhs = rhs;
            binop->op = op.str();

            lhs = binop;
        }
    }

    static Expr* parseExpr(ParserState* ps)
    {
        return parseRhs(ps, parsePrimary(ps), 0);
    }

    static Expr* parseCondExpr(ParserState* ps, int paren_nesting = 0);


    // parse `(cond_expr) && (cond_expr)` and friends.
    // but not `! (cond_expr)`
    static Expr* parseBinaryCondExpr(ParserState* ps, Expr* lhs)
    {
        assert(lhs != nullptr);

        std::string op;
        if(auto tok = ps->next(); tok == TT::LogicalAnd)
            op = "&&";
        else if(tok == TT::LogicalOr)
            op = "||";
        else
            throw util::ParseException("simple::parser", "expected either '&&' or '||', found '{}' instead", tok.text);

        if(auto n = ps->next(); n != TT::LParen)
            throw util::ParseException("simple::parser", "expected '(' after '{}', found '{}' instead", op, n.text);

        auto rhs = parseCondExpr(ps);

        if(auto n = ps->next(); n != TT::RParen)
            throw util::ParseException("simple::parser", "expected ')' after expression, found '{}' instead", n.text);

        auto ret = new BinaryOp();
        ret->lhs = lhs;
        ret->rhs = rhs;
        ret->op = op;
        return ret;
    }


    // parse `rel_expr < rel_expr` and friends.
    static Expr* parseRelationalExpr(ParserState* ps, Expr* lhs)
    {
        assert(lhs != nullptr);
        std::string op;
        if(auto tok = ps->next(); tok == TT::LAngle)
            op = "<";
        else if(tok == TT::RAngle)
            op = ">";
        else if(tok == TT::GreaterEqual)
            op = ">=";
        else if(tok == TT::LessEqual)
            op = "<=";
        else if(tok == TT::NotEqual)
            op = "!=";
        else if(tok == TT::EqualsTo)
            op = "==";
        else
            throw util::ParseException("simple::parser", "invalid relational operator '{}'", tok.text);

        auto rhs = parseExpr(ps);
        assert(rhs);

        auto ret = new BinaryOp();
        ret->lhs = lhs;
        ret->rhs = rhs;
        ret->op = op;
        return ret;
    }




    static Expr* parseCondExpr(ParserState* ps, int paren_nesting)
    {
        if(ps->peek() == TT::Exclamation)
        {
            ps->next();
            if(auto n = ps->next(); n != TT::LParen)
                throw util::ParseException("simple::parser", "expected '(' after '!', found '{}' instead", n.text);

            auto ret = new UnaryOp();
            ret->op = "!";
            ret->expr = parseCondExpr(ps, paren_nesting + 1);

            if(ps->next() != TT::RParen)
                throw util::ParseException("simple::parser", "expected ')' to match a '('");

            return ret;
        }
        else if(ps->peek() == TT::LParen)
        {
            // consume the paren
            ps->next();

            auto is_relational_expr = [](Expr* expr) -> bool {
                if(auto x = dynamic_cast<BinaryOp*>(expr); x && BinaryOp::isRelational(x->op))
                    return true;
                return false;
            };

            /*
                due to the poor design of this "simple" grammar, parsing becomes complicated.
                it would be easy if conditional expressions were just normal expressions, but NOOoOOOooo

                a "cond_expr" (that doesn't start with '!') is either "(cond_expr) && (cond_expr)", or
                "(cond_expr) || (cond_expr)". the problem is that '(' can also start an expression, but
                in a cond_expr, "normal" expressions can only be part of relational expressions (eg.
                "expr < expr" or whatever).

                for example, "(1 + 2) < 3" is a valid cond_expr; naively parsing a cond_expr
                upon seeing the '(' instead of parsing a rel_expr will end in failure.

                so what we do is parse a cond_expr as the lhs first; this should allow us to recursively
                handle the case where it's actually a rel_expr inside the '()'.
            */

            auto lhs = parseCondExpr(ps, paren_nesting + 1);
            assert(lhs);

            if(auto n = ps->next(); n != TT::RParen)
                throw util::ParseException("simple::parser", "expected ')', found '{}'", n.text);

            // continue parsing an expression if we can...
            if(auto n = ps->peek(); n != TT::RParen && get_precedence(n) != -1)
                lhs = parseRhs(ps, lhs, 0);

            // now for some hackery...
            if(ps->peek() == TT::RParen && paren_nesting > 0)
            {
                // we are in a nested '()', so just return here.
                return lhs;
            }
            else if(BinaryOp::isRelational(ps->peek().text))
            {
                if(is_relational_expr(lhs))
                    throw util::ParseException("simple::parser", "relational operators cannot be chained");

                return parseRelationalExpr(ps, lhs);

                // return lhs.flatmap([&ps, &is_relational_expr ](auto lhs) -> auto {
                //     if(is_relational_expr(lhs))
                //         return Result<Expr*>(ErrFmt("relational operators cannot be chained"));

                //     return parseRelationalExpr(ps, lhs);
                // });
            }
            else if(BinaryOp::isConditional(ps->peek().text))
            {
                return parseBinaryCondExpr(ps, lhs);
                // return lhs.flatmap([&ps](auto lhs) -> auto { return parseBinaryCondExpr(ps, lhs); });
            }
            else
            {
                throw util::ParseException("simple::parser", "expected a conditional/relational operator after ')', found '{}'", ps->peek().text);
            }
        }
        else
        {
            auto lhs = parseExpr(ps);
            assert(lhs);

            // peek the next token. if it's a closing paren, then we defer to the higher-level.
            if(ps->peek() == TT::RParen && paren_nesting > 0)
                return lhs;

            // this *has* to be a rel_expr (since cond_exprs must be parenthesised)
            return parseRelationalExpr(ps, lhs);

            // return lhs.flatmap([&ps](auto lhs) -> auto { return parseRelationalExpr(ps, lhs); });
        }
    }

    static Stmt* parseStmt(ParserState* ps);
    static StmtList parseStatementList(ParserState* ps)
    {
        StmtList list {};

        if(ps->next() != TT::LBrace)
            throw util::ParseException("simple::parser", "expected '{'");

        while(ps->peek() != TT::RBrace)
        {
            if(ps->peek() == TT::EndOfFile)
                throw util::ParseException("simple::parser", "unexpected end of file (expected '}')");

            list.statements.push_back(parseStmt(ps));
        }

        if(ps->next() != TT::RBrace)
            throw util::ParseException("simple::parser", "expected '}'");

        // the grammar specifies "stmt+"
        if(list.statements.empty())
            throw util::ParseException("simple::parser", "expected at least one statement between '{' and '}'");

        return list;
    }

    static IfStmt* parseIfStmt(ParserState* ps)
    {
        // note: 'if' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            throw util::ParseException("simple::parser", "expected '(' after 'if'");

        auto ret = new IfStmt();
        ret->condition = parseCondExpr(ps);

        if(auto n = ps->next(); n != TT::RParen)
            throw util::ParseException("simple::parser", "expected ')' after conditional, found '{}' instead", n.text);

        if(auto then = ps->next(); then != TT::Identifier || then.text != KW_Then)
            throw util::ParseException("simple::parser", "expected 'then' after condition for 'if'");

        ret->true_case = parseStatementList(ps);

        if(auto e = ps->next(); e != TT::Identifier || e.text != KW_Else)
            throw util::ParseException("simple::parser", "'else' clause is mandatory");

        ret->false_case = parseStatementList(ps);
        return ret;
    }

    static WhileLoop* parseWhileLoop(ParserState* ps)
    {
        // note: 'while' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            throw util::ParseException("simple::parser", "expected '(' after 'while'");

        auto ret = new WhileLoop();
        ret->condition = parseCondExpr(ps);

        if(auto n = ps->next(); n != TT::RParen)
            throw util::ParseException("simple::parser", "expected ')' after conditional, found '{}' instead", n.text);

        ret->body = parseStatementList(ps);
        return ret;
    }

    static Stmt* parseStmt(ParserState* ps)
    {
        auto check_semicolon = [](ParserState* ps, auto ret) -> auto {
            if(auto n = ps->next(); n != TT::Semicolon)
                throw util::ParseException("simple::parser", "expected semicolon after statement, found '{}' instead", n.text);
            else
                return ret;
        };

        // this just needs 1 token of lookahead; if the next token is a '=', then
        // this is an assignment, and the identifier on the left is a variable -- even
        // if it's a keyword.
        auto tok = ps->next();
        auto match_keyword_if_not_assign = [&ps](const Token& tok, const auto& keyword) -> bool {
            return (tok == TT::Identifier && ps->peek() != TT::Equal && tok.text == keyword);
        };

        if(match_keyword_if_not_assign(tok, KW_Read))
        {
            auto read = new ReadStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                throw util::ParseException("simple::parser", "expected identifier after 'read'");
            else
                read->var_name = name.text.str();

            return check_semicolon(ps, read);
        }
        else if(match_keyword_if_not_assign(tok, KW_Print))
        {
            auto print = new PrintStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                throw util::ParseException("simple::parser", "expected identifier after 'print'");
            else
                print->var_name = name.text.str();

            return check_semicolon(ps, print);
        }
        else if(match_keyword_if_not_assign(tok, KW_Call))
        {
            auto call = new ProcCall();
            if(auto name = ps->next(); name != TT::Identifier)
                throw util::ParseException("simple::parser", "expected identifier after 'call'");
            else
                call->proc_name = name.text.str();

            return check_semicolon(ps, call);
        }
        else if(match_keyword_if_not_assign(tok, KW_If))
        {
            return parseIfStmt(ps);
        }
        else if(match_keyword_if_not_assign(tok, KW_While))
        {
            return parseWhileLoop(ps);
        }
        else if(tok == TT::Identifier)
        {
            // based on the grammar, we know that statements starting with an identifier
            // (that is not one of the control flow keywords) will be an assignment.
            if(ps->next() != TT::Equal)
                throw util::ParseException("simple::parser", "expected '=' after identifier");

            auto assign = new AssignStmt();
            assign->lhs = tok.text.str();
            assign->rhs = parseExpr(ps);

            return check_semicolon(ps, assign);
        }
        else
        {
            throw util::ParseException("simple::parser", "unexpected token '{}' at beginning of statement", tok.text);
        }
    }

    static Procedure* parseProcedure(ParserState* ps)
    {
        if(auto kw = ps->next(); kw != TT::Identifier || kw.text != KW_Procedure)
            throw util::ParseException("simple::parser", "expected 'procedure' to define a procedure (found '{}')", kw.text);

        auto proc = new Procedure();

        if(auto name = ps->next(); name == TT::Identifier)
            proc->name = name.text.str();
        else
            throw util::ParseException("simple::parser", "expected identifier after 'procedure'");

        proc->body = parseStatementList(ps);
        return proc;
    }

    Program* parseProgram(zst::str_view input)
    {
        auto ps = ParserState { input };
        auto prog = new Program();

        for(TokenType t; (t = ps.peek()) != TT::EndOfFile;)
            prog->procedures.push_back(parseProcedure(&ps));

        return prog;
    }

    Expr* parseExpression(zst::str_view input)
    {
        auto ps = ParserState { input };
        auto ret = parseExpr(&ps);
        assert(ret);

        if(auto tmp = ps.next(); tmp != TT::EndOfFile)
            throw util::ParseException("simple::parser", "unexpected token '{}' after expression", tmp.text);

        return ret;
    }
}
