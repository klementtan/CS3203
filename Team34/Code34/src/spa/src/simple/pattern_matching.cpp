#include "simple/ast.h"

namespace simple::ast
{
    bool exactMatch(const Expr* subtree, const Expr* tree)
    {
        if(auto var0 = dynamic_cast<const VarRef*>(subtree))
        {
            auto var1 = dynamic_cast<const VarRef*>(tree);
            return var1 && var0->name == var1->name;
        }

        if(auto constant0 = dynamic_cast<const Constant*>(subtree))
        {
            auto constant1 = dynamic_cast<const Constant*>(tree);
            return constant1 && constant0->value == constant1->value;
        }

        if(auto binary0 = dynamic_cast<const BinaryOp*>(subtree))
        {
            auto binary1 = dynamic_cast<const BinaryOp*>(tree);
            return binary1 && binary0->op == binary1->op && exactMatch(binary0->lhs.get(), binary1->lhs.get()) &&
                   exactMatch(binary0->rhs.get(), binary1->rhs.get());
        }

        return false;
    }

    bool partialMatch(const Expr* subtree, const Expr* tree)
    {
        if(exactMatch(subtree, tree))
            return true;

        if(auto binary_tree = dynamic_cast<const BinaryOp*>(tree))
            return partialMatch(subtree, binary_tree->lhs.get()) || partialMatch(subtree, binary_tree->rhs.get());

        return false;
    }
}
