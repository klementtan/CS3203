// ast.cpp
// simply a file where the definitions of AST helper methods sit.

#include <zpr.h>
#include <zst.h>
#include <string>

#include "simple/ast.h"

namespace simple::ast
{
    Stmt::~Stmt() { }

    Expr::~Expr() { }

    std::string VarRef::toString() const
    {
        return this->name;
    }

    std::string Constant::toString() const
    {
        return zpr::sprint("{}", this->value);
    }

    std::string BinaryOp::toString() const
    {
        return zpr::sprint("({} {} {})", this->lhs->toString(), this->op, this->rhs->toString());
    }

    std::string UnaryOp::toString() const
    {
        return zpr::sprint("{}{}", this->op, this->expr->toString());
    }

    static constexpr int INDENT_WIDTH = 4;
    static constexpr int GUTTER_WIDTH = 3;

    std::string StmtList::toString(int nesting) const
    {
        auto ret = zpr::sprint("{}{\n", zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""));
        for(const auto& stmt : this->statements)
            ret += zpr::sprint("{02} {}", stmt->id, stmt->toString(nesting + 1));

        ret += zpr::sprint("{}}\n", zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""));
        return ret;
    }

    std::string IfStmt::toString(int nesting) const
    {
        return zpr::sprint("{}if{} then\n{}{}else\n{}", zpr::w(nesting * INDENT_WIDTH)(""), this->condition->toString(),
            this->true_case.toString(nesting), zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""),
            this->false_case.toString(nesting));
    }

    std::string ProcCall::toString(int nesting) const
    {
        return zpr::sprint("{}call {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->proc_name);
    }

    std::string WhileLoop::toString(int nesting) const
    {
        return zpr::sprint("{}while{}\n{}", zpr::w(nesting * INDENT_WIDTH)(""), this->condition->toString(),
            this->body.toString(nesting));
    }

    std::string AssignStmt::toString(int nesting) const
    {
        return zpr::sprint("{}{} = {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->lhs, this->rhs->toString());
    }

    std::string ReadStmt::toString(int nesting) const
    {
        return zpr::sprint("{}read {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->var_name);
    }

    std::string PrintStmt::toString(int nesting) const
    {
        return zpr::sprint("{}print {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->var_name);
    }

    std::string Procedure::toString() const
    {
        return zpr::sprint("{}procedure {}\n{}", zpr::w(GUTTER_WIDTH)(""), this->name, this->body.toString(0));
    }

    std::string Program::toString() const
    {
        std::string ret;
        for(const auto& proc : this->procedures)
            ret += proc->toString();
        return ret;
    }

    template <typename T>
    T remove_gutter(T beg, T end)
    {
        T dest = beg;
        T itr = beg;
        int i = GUTTER_WIDTH;
        while(itr + 1 != end && i)
        {
            itr++;
            i--;
        }
        for(; itr != end; ++itr)
        {
            *(dest++) = *itr;
            if(*itr == '\n')
            {
                int i = GUTTER_WIDTH;
                while(itr + 1 != end && i)
                {
                    itr++;
                    i--;
                }
            }
        }
        return dest;
    }

    std::string Program::toProgFormat() const
    {
        std::string str = this->toString();
        str.erase(remove_gutter(str.begin(), str.end()), str.end());
        return str;
    }
}
