#include "simple/ast.h"

namespace util
{
    namespace s_ast = simple::ast;

    bool _eq(s_ast::Expr* e0, s_ast::Expr* e1)
    {
        if(s_ast::VarRef* var0 = dynamic_cast<s_ast::VarRef*>(e0))
        {
            s_ast::VarRef* var1 = dynamic_cast<s_ast::VarRef*>(e1);
            return var1 && var0->name == var1->name;
        }

        if(s_ast::Constant* constant0 = dynamic_cast<s_ast::Constant*>(e0))
        {
            s_ast::Constant* constant1 = dynamic_cast<s_ast::Constant*>(e1);
            return constant1 && constant0->value == constant1->value;
        }

        if(s_ast::BinaryOp* binary0 = dynamic_cast<s_ast::BinaryOp*>(e0))
        {
            s_ast::BinaryOp* binary1 = dynamic_cast<s_ast::BinaryOp*>(e1);
            return binary1 && binary0->op == binary1->op && _eq(binary0->lhs, binary1->lhs) &&
                  _eq(binary0->rhs, binary1->rhs);
        }

        return false;
    }

    bool matches(s_ast::Expr* subtree, s_ast::Expr* tree)
    {
        if(_eq(subtree, tree))
            return true;

        if(s_ast::BinaryOp* binary_tree = dynamic_cast<s_ast::BinaryOp*>(tree))
            return matches(subtree, binary_tree->lhs) || matches(subtree, binary_tree->rhs);

        return false;
    }
}