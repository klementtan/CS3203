#include "simple/ast.h"

namespace simple::ast
{
    bool exactMatch(Expr* subtree, Expr* tree)
    {
        if(auto var0 = dynamic_cast<VarRef*>(subtree))
        {
            auto var1 = dynamic_cast<VarRef*>(tree);
            return var1 && var0->name == var1->name;
        }

        if(auto constant0 = dynamic_cast<Constant*>(subtree))
        {
            auto constant1 = dynamic_cast<Constant*>(tree);
            return constant1 && constant0->value == constant1->value;
        }

        if(auto binary0 = dynamic_cast<BinaryOp*>(subtree))
        {
            auto binary1 = dynamic_cast<BinaryOp*>(tree);
            return binary1 && binary0->op == binary1->op && exactMatch(binary0->lhs.get(), binary1->lhs.get()) &&
                   exactMatch(binary0->rhs.get(), binary1->rhs.get());
        }

        return false;
    }

    bool partialMatch(Expr* subtree, Expr* tree)
    {
        if(exactMatch(subtree, tree))
            return true;

        if(BinaryOp* binary_tree = dynamic_cast<BinaryOp*>(tree))
            return partialMatch(subtree, binary_tree->lhs.get()) || partialMatch(subtree, binary_tree->rhs.get());

        return false;
    }
}
