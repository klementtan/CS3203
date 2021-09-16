// stringify.cpp

#include <zpr.h>

#include "simple/ast.h"

namespace simple::ast
{
    static bool is_binop(const Expr* e)
    {
        return dynamic_cast<const BinaryOp*>(e) != nullptr;
    }

    static bool is_conditional_op(const Expr* e)
    {
        auto bin = dynamic_cast<const BinaryOp*>(e);
        return bin && BinaryOp::isConditional(bin->op);
    }

    static std::string parenthesise_conditional(const Expr* cond)
    {
        if(is_conditional_op(cond) || !is_binop(cond))
            return zpr::sprint("({})", cond->toString());

        else
            return cond->toString();
    }

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
        if(is_conditional_op(this))
        {
            auto left = parenthesise_conditional(this->lhs);
            auto right = parenthesise_conditional(this->rhs);

            return zpr::sprint("{} {} {}", left, this->op, right);
        }
        else
        {
            return zpr::sprint("({} {} {})", this->lhs->toString(), this->op, this->rhs->toString());
        }
    }

    std::string UnaryOp::toString() const
    {
        if(is_conditional_op(this->expr) || !is_binop(this->expr))
            return zpr::sprint("{}({})", this->op, this->expr->toString());
        else
            return zpr::sprint("{}{}", this->op, this->expr->toString());
    }

    static constexpr int INDENT_WIDTH = 4;
    static constexpr int GUTTER_WIDTH = 3;

    std::string StmtList::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            std::string ret {};
            for(const auto& stmt : this->statements)
                ret += stmt->toString(0, /* compact: */ true);
            return "{" + ret + "}";
        }
        else
        {
            auto ret = zpr::sprint("{}{\n", zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""));
            for(const auto& stmt : this->statements)
                ret += zpr::sprint("{02} {}", stmt->id, stmt->toString(nesting + 1));

            ret += zpr::sprint("{}}\n", zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""));
            return ret;
        }
    }

    std::string IfStmt::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("if{}then{}else{}", parenthesise_conditional(this->condition),
                this->true_case.toString(0, true), this->false_case.toString(0, true));
        }
        else
        {
            return zpr::sprint("{}if({}) then\n{}{}else\n{}", zpr::w(nesting * INDENT_WIDTH)(""),
                this->condition->toString(), this->true_case.toString(nesting),
                zpr::w(GUTTER_WIDTH + nesting * INDENT_WIDTH)(""), this->false_case.toString(nesting));
        }
    }

    std::string ProcCall::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("call {};", this->proc_name);
        }
        else
        {
            return zpr::sprint("{}call {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->proc_name);
        }
    }

    std::string WhileLoop::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("while{}{}", parenthesise_conditional(this->condition), this->body.toString(0, true));
        }
        else
        {
            return zpr::sprint("{}while({})\n{}", zpr::w(nesting * INDENT_WIDTH)(""), this->condition->toString(),
                this->body.toString(nesting));
        }
    }

    std::string AssignStmt::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("{} = {};", this->lhs, this->rhs->toString());
        }
        else
        {
            return zpr::sprint("{}{} = {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->lhs, this->rhs->toString());
        }
    }

    std::string ReadStmt::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("read {};", this->var_name);
        }
        else
        {
            return zpr::sprint("{}read {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->var_name);
        }
    }

    std::string PrintStmt::toString(int nesting, bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("print {};", this->var_name);
        }
        else
        {
            return zpr::sprint("{}print {};\n", zpr::w(nesting * INDENT_WIDTH)(""), this->var_name);
        }
    }

    std::string Procedure::toString(bool compact) const
    {
        if(compact)
        {
            return zpr::sprint("procedure {}{}", this->name, this->body.toString(0, true));
        }
        else
        {
            return zpr::sprint("{}procedure {}\n{}", zpr::w(GUTTER_WIDTH)(""), this->name, this->body.toString(0));
        }
    }

    std::string Program::toString(bool compact) const
    {
        std::string ret;
        for(const auto& proc : this->procedures)
            ret += proc->toString(compact);
        return ret;
    }

    bool BinaryOp::isRelational(zst::str_view op)
    {
        return op == "<" || op == ">" || op == ">=" || op == "<=" || op == "==" || op == "!=";
    }

    bool BinaryOp::isConditional(zst::str_view op)
    {
        return op == "&&" || op == "||";
    }
}
