// parser.cpp

#include <zpr.h>
#include <zst.h>

#include "ast.h"
#include "simple_parser.h"

namespace simple_parser
{
    using namespace ast;

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


    template <typename... Args>
    [[noreturn]] static void parse_error(const char* fmt, Args&&... xs)
    {
        zpr::fprintln(stderr, "parse error: {}", zpr::fwd(fmt, static_cast<Args&&>(xs)...));
        exit(1);
    }

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
                parse_error("expected ')'");

            return ret;
        }
        else if(ps->peek() == TT::Number)
        {
            auto num = ps->next().text;
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
        else
        {
            parse_error("invalid start of expression with '{}'", ps->peek().text);
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

            auto next = get_precedence(ps->peek());
            if(next > prec)
                rhs = parseRhs(ps, rhs, prec + 1);

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

    // this is fully parenthesised, so life is easier
    static Expr* parseCondExpr(ParserState* ps)
    {
        if(ps->peek() == TT::Exclamation)
        {
            ps->next();
            if(ps->next() != TT::LParen)
                parse_error("expected '(' after '!'");

            auto ret = new UnaryOp();
            ret->expr = parseCondExpr(ps);
            ret->op = "!";

            if(ps->next() != TT::RParen)
                parse_error("expected ')' to match a '('");

            return ret;
        }
        else if(ps->peek() == TT::LParen)
        {
            // this is either (expr) || (expr) or (expr) && (expr).

            auto parse_parenthesised_condexpr = [](ParserState* ps) -> Expr* {
                if(ps->next() != TT::LParen)
                    parse_error("expected '('");

                auto ret = parseCondExpr(ps);
                if(ps->next() != TT::RParen)
                    parse_error("expected ')' to match a '('");

                return ret;
            };

            auto lhs = parse_parenthesised_condexpr(ps);

            std::string op;
            if(auto tok = ps->next(); tok == TT::LogicalAnd)
                op = "&&";
            else if(tok == TT::LogicalOr)
                op = "||";
            else
                parse_error("expected either '&&' or '||'");

            auto rhs = parse_parenthesised_condexpr(ps);

            auto ret = new BinaryOp();
            ret->lhs = lhs;
            ret->rhs = rhs;
            ret->op = op;
            return ret;
        }
        else
        {
            auto lhs = parseExpr(ps);
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
                parse_error("invalid binary operator '{}'", tok.text);

            auto rhs = parseExpr(ps);

            auto ret = new BinaryOp();
            ret->lhs = lhs;
            ret->rhs = rhs;
            ret->op = op;
            return ret;
        }
    }

    static Stmt* parseStmt(ParserState* ps);
    static StmtList parseStatementList(ParserState* ps)
    {
        StmtList list {};

        if(ps->next() != TT::LBrace)
            parse_error("expected '{'");

        while(ps->peek() != TT::RBrace)
        {
            if(ps->peek() == TT::EndOfFile)
                parse_error("unexpected end of file");

            list.statements.push_back(parseStmt(ps));
        }

        if(ps->next() != TT::RBrace)
            parse_error("expected '}'");

        // the grammar specifies "stmt+"
        if(list.statements.empty())
            parse_error("expected at least one statement between '{' and '}'");

        return list;
    }

    static IfStmt* parseIfStmt(ParserState* ps)
    {
        // note: 'if' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            parse_error("expected '(' after 'if'");

        auto ret = new IfStmt();
        ret->condition = parseCondExpr(ps);

        if(ps->next() != TT::RParen)
            parse_error("expected ')'");


        if(auto then = ps->next(); then != TT::Identifier || then.text != KW_Then)
            parse_error("expected 'then' after condition for 'if'");

        ret->true_case = parseStatementList(ps);

        if(auto e = ps->next(); e != TT::Identifier || e.text != KW_Else)
            parse_error("'else' clause is mandatory");

        ret->false_case = parseStatementList(ps);

        return ret;
    }

    static WhileLoop* parseWhileLoop(ParserState* ps)
    {
        // note: 'while' was already eaten, so we need to parse the expression immediately.
        if(ps->next() != TT::LParen)
            parse_error("expected '(' after 'while'");

        auto ret = new WhileLoop();
        ret->condition = parseCondExpr(ps);

        if(ps->next() != TT::RParen)
            parse_error("expected ')'");

        ret->body = parseStatementList(ps);
        return ret;
    }

    static Stmt* parseStmt(ParserState* ps)
    {
        auto check_semicolon = [](ParserState* ps) {
            if(ps->next() != TT::Semicolon)
                parse_error("expected semicolon after statement");
        };

        if(auto tok = ps->next(); tok == TT::Identifier && tok.text == KW_Read)
        {
            auto read = new ReadStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                parse_error("expected identifier after 'read'");
            else
                read->var_name = name.text.str();

            check_semicolon(ps);
            return read;
        }
        else if(tok == TT::Identifier && tok.text == KW_Print)
        {
            auto print = new PrintStmt();
            if(auto name = ps->next(); name != TT::Identifier)
                parse_error("expected identifier after 'print'");
            else
                print->var_name = name.text.str();

            check_semicolon(ps);
            return print;
        }
        else if(tok == TT::Identifier && tok.text == KW_Call)
        {
            auto call = new ProcCall();
            if(auto name = ps->next(); name != TT::Identifier)
                parse_error("expected identifier after 'call'");
            else
                call->proc_name = name.text.str();

            check_semicolon(ps);
            return call;
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
                parse_error("expected '='");

            auto assign = new AssignStmt();
            assign->lhs = tok.text.str();
            assign->rhs = parseExpr(ps);

            check_semicolon(ps);
            return assign;
        }
        else
        {
            parse_error("unexpected token '{}' at beginning of statement", tok.text);
        }
    }

    static Procedure* parseProcedure(ParserState* ps)
    {
        if(auto kw = ps->next(); kw != TT::Identifier || kw.text != KW_Procedure)
            parse_error("expected 'procedure' to define a procedure (found '{}')", kw.text);

        auto proc = new Procedure();

        if(auto name = ps->next(); name == TT::Identifier)
            proc->name = name.text.str();
        else
            parse_error("expected identifier after 'procedure' keyword");

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
}
