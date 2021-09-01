// parser.cpp

#include <zpr.h>
#include <zst.h>

#include "ast.h"
#include "util.h"
#include "simple_parser.h"

namespace simple_parser
{
    using namespace ast;

    using zst::Ok;
    using zst::Err;
    using zst::ErrFmt;

    template <typename T>
    using Result = zst::Result<T, std::string>;

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

    static Result<Expr*> parseExpr(ParserState* ps);
    static Result<Expr*> parsePrimary(ParserState* ps)
    {
        if(ps->peek() == TT::LParen)
        {
            ps->next();
            auto ret = parseExpr(ps);
            if(ps->next() != TT::RParen)
                return ErrFmt("expected ')'");

            return ret;
        }
        else if(ps->peek() == TT::Number)
        {
            auto num = ps->next().text;
            auto constant = new Constant();
            for(char c : num)
                constant->value = 10 * constant->value + (c - '0');

            return Ok(constant);
        }
        else if(ps->peek() == TT::Identifier)
        {
            auto vr = new VarRef();
            vr->name = ps->next().text.str();

            return Ok(vr);
        }
        else
        {
            return ErrFmt("invalid start of expression with '{}'", ps->peek().text);
        }
    }

    static Result<Expr*> parseRhs(ParserState* ps, Expr* lhs, int priority)
    {
        if(priority == -1)
            return Ok(lhs);

        // everything here is left-associative, so there is no problem.
        while(true)
        {
            auto prec = get_precedence(ps->peek());
            if(prec < priority)
                return Ok(lhs);

            auto op = ps->next().text;
            auto rhs = parsePrimary(ps);
            if(!rhs)
                return rhs;

            auto next = get_precedence(ps->peek());
            if(next > prec)
                rhs = parseRhs(ps, rhs.unwrap(), prec + 1);

            auto binop = new BinaryOp();
            binop->lhs = lhs;
            binop->rhs = rhs.unwrap();
            binop->op = op.str();

            lhs = binop;
        }
    }

    static Result<Expr*> parseExpr(ParserState* ps)
    {
        if(auto pri = parsePrimary(ps); pri.ok())
            return parseRhs(ps, pri.unwrap(), 0);
        else
            return pri;
    }

    // this is fully parenthesised, so life is easier
    static Result<Expr*> parseCondExpr(ParserState* ps)
    {
        if(ps->peek() == TT::Exclamation)
        {
            ps->next();
            if(ps->next() != TT::LParen)
                return ErrFmt("expected '(' after '!'");

            auto ret = new UnaryOp();
            if(auto cond = parseCondExpr(ps); cond.ok())
                ret->expr = cond.unwrap();
            else
                return cond;

            ret->op = "!";
            if(ps->next() != TT::RParen)
                return ErrFmt("expected ')' to match a '('");

            return Ok(ret);
        }
        else if(ps->peek() == TT::LParen)
        {
            // this is either (expr) || (expr) or (expr) && (expr).

            auto parse_parenthesised_condexpr = [](ParserState* ps) -> Result<Expr*> {
                if(ps->next() != TT::LParen)
                    return ErrFmt("expected '('");

                auto ret = parseCondExpr(ps);
                if(ps->next() != TT::RParen)
                    return ErrFmt("expected ')' to match a '('");

                return ret;
            };

            auto lhs = parse_parenthesised_condexpr(ps);
            if(!lhs.ok())
                return Err(lhs.error());

            std::string op;
            if(auto tok = ps->next(); tok == TT::LogicalAnd)
                op = "&&";
            else if(tok == TT::LogicalOr)
                op = "||";
            else
                return ErrFmt("expected either '&&' or '||'");

            auto rhs = parse_parenthesised_condexpr(ps);
            if(!rhs.ok())
                return Err(rhs.error());

            auto ret = new BinaryOp();
            ret->lhs = lhs.unwrap();
            ret->rhs = rhs.unwrap();
            ret->op = op;
            return Ok(ret);
        }
        else
        {
            auto lhs = parseExpr(ps);
            if(!lhs.ok())
                return lhs;

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
                return ErrFmt("invalid binary operator '{}'", tok.text);

            auto rhs = parseExpr(ps);
            if(!rhs.ok())
                return rhs;

            auto ret = new BinaryOp();
            ret->lhs = lhs.unwrap();
            ret->rhs = rhs.unwrap();
            ret->op = op;
            return Ok(ret);
        }
    }

    static Result<Stmt*> parseStmt(ParserState* ps);
    static Result<StmtList> parseStatementList(ParserState* ps)
    {
        StmtList list {};

        if(ps->next() != TT::LBrace)
            return ErrFmt("expected '{'");

        while(ps->peek() != TT::RBrace)
        {
            if(ps->peek() == TT::EndOfFile)
                return ErrFmt("unexpected end of file (expected '}')");

            if(auto s = parseStmt(ps); s.ok())
                list.statements.push_back(s.unwrap());
            else
                return Err(s.error());
        }

        if(ps->next() != TT::RBrace)
            return ErrFmt("expected '}'");

        // the grammar specifies "stmt+"
        if(list.statements.empty())
            return ErrFmt("expected at least one statement between '{' and '}'");

        return Ok(list);
    }

    static Result<IfStmt*> parseIfStmt(ParserState* ps)
    {
        // note: 'if' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            return ErrFmt("expected '(' after 'if'");

        auto ret = new IfStmt();
        if(auto cond = parseCondExpr(ps); cond.ok())
            ret->condition = cond.unwrap();
        else
            return Err(cond.error());

        if(ps->next() != TT::RParen)
            return ErrFmt("expected ')'");

        if(auto then = ps->next(); then != TT::Identifier || then.text != KW_Then)
            return ErrFmt("expected 'then' after condition for 'if'");

        if(auto tc = parseStatementList(ps); tc.ok())
            ret->true_case = tc.unwrap();
        else
            return Err(tc.error());

        if(auto e = ps->next(); e != TT::Identifier || e.text != KW_Else)
            return ErrFmt("'else' clause is mandatory");

        if(auto fc = parseStatementList(ps); fc.ok())
            ret->false_case = fc.unwrap();
        else
            return Err(fc.error());

        return Ok(ret);
    }

    static Result<WhileLoop*> parseWhileLoop(ParserState* ps)
    {
        // note: 'while' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            return ErrFmt("expected '(' after 'while'");

        auto ret = new WhileLoop();
        if(auto cond = parseCondExpr(ps); cond.ok())
            ret->condition = cond.unwrap();
        else
            return Err(cond.error());

        if(ps->next() != TT::RParen)
            return ErrFmt("expected ')'");

        if(auto body = parseStatementList(ps); body.ok())
            ret->body = body.unwrap();
        else
            return Err(body.error());

        return Ok(ret);
    }

    static Result<Stmt*> parseStmt(ParserState* ps)
    {
        auto check_semicolon = [](ParserState* ps, auto ret) -> zst::Result<Stmt*, std::string> {
            if(ps->next() != TT::Semicolon)
                return ErrFmt("expected semicolon after statement");
            else
                return Ok(ret);
        };

        if(auto tok = ps->next(); tok == TT::Identifier && tok.text == KW_Read)
        {
            auto read = new ReadStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                return ErrFmt("expected identifier after 'read'");
            else
                read->var_name = name.text.str();

            return check_semicolon(ps, read);
        }
        else if(tok == TT::Identifier && tok.text == KW_Print)
        {
            auto print = new PrintStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                return ErrFmt("expected identifier after 'print'");
            else
                print->var_name = name.text.str();

            return check_semicolon(ps, print);
        }
        else if(tok == TT::Identifier && tok.text == KW_Call)
        {
            auto call = new ProcCall();
            if(auto name = ps->next(); name != TT::Identifier)
                return ErrFmt("expected identifier after 'call'");
            else
                call->proc_name = name.text.str();

            return check_semicolon(ps, call);
        }
        else if(tok == TT::Identifier && tok.text == KW_If)
        {
            return parseIfStmt(ps);
        }
        else if(tok == TT::Identifier && tok.text == KW_While)
        {
            return parseWhileLoop(ps);
        }
        else if(tok == TT::Identifier)
        {
            // based on the grammar, we know that statements starting with an identifier
            // (that is not one of the control flow keywords) will be an assignment.
            if(ps->next() != TT::Equal)
                return ErrFmt("expected '=' after identifier");

            auto assign = new AssignStmt();
            assign->lhs = tok.text.str();

            if(auto rhs = parseExpr(ps); rhs.ok())
                assign->rhs = rhs.unwrap();
            else
                return Err(rhs.error());

            return check_semicolon(ps, assign);
        }
        else
        {
            return ErrFmt("unexpected token '{}' at beginning of statement", tok.text);
        }
    }

    static Result<Procedure*> parseProcedure(ParserState* ps)
    {
        if(auto kw = ps->next(); kw != TT::Identifier || kw.text != KW_Procedure)
            return ErrFmt("expected 'procedure' to define a procedure (found '{}')", kw.text);

        auto proc = new Procedure();

        if(auto name = ps->next(); name == TT::Identifier)
            proc->name = name.text.str();
        else
            return ErrFmt("expected identifier after 'procedure' keyword");

        if(auto body = parseStatementList(ps); body.ok())
            proc->body = body.unwrap();
        else
            return Err(body.error());

        return Ok(proc);
    }

    Result<Program*> parseProgram(zst::str_view input)
    {
        auto ps = ParserState { input };
        auto prog = new Program();

        for(TokenType t; (t = ps.peek()) != TT::EndOfFile;)
        {
            if(auto p = parseProcedure(&ps); p.ok())
                prog->procedures.push_back(p.unwrap());
            else
                return Err(p.error());
        }

        return Ok(prog);
    }

    Result<Expr*> parseExpression(zst::str_view input)
    {
        auto ps = ParserState { input };
        auto ret = parseExpr(&ps);

        if(auto tmp = ps.next(); tmp != TT::EndOfFile)
            return ErrFmt("unexpected token '{}' after expression", tmp.text);

        return ret;
    }
}
