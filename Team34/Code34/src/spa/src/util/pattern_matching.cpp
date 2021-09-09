#include "simple/ast.h"

namespace util
{
    namespace s_ast = simple::ast;

    bool exact_match(s_ast::Expr* subtree, s_ast::Expr* tree)
    {
        if(s_ast::VarRef* var0 = dynamic_cast<s_ast::VarRef*>(subtree))
        {
            s_ast::VarRef* var1 = dynamic_cast<s_ast::VarRef*>(tree);
            return var1 && var0->name == var1->name;
        }

        if(s_ast::Constant* constant0 = dynamic_cast<s_ast::Constant*>(subtree))
        {
            s_ast::Constant* constant1 = dynamic_cast<s_ast::Constant*>(tree);
            return constant1 && constant0->value == constant1->value;
        }

        if(s_ast::BinaryOp* binary0 = dynamic_cast<s_ast::BinaryOp*>(subtree))
        {
            s_ast::BinaryOp* binary1 = dynamic_cast<s_ast::BinaryOp*>(tree);
            return binary1 && binary0->op == binary1->op && exact_match(binary0->lhs, binary1->lhs) &&
                   exact_match(binary0->rhs, binary1->rhs);
        }

        return false;
    }

    bool partial_match(s_ast::Expr* subtree, s_ast::Expr* tree)
    {
        if(exact_match(subtree, tree))
            return true;

        if(s_ast::BinaryOp* binary_tree = dynamic_cast<s_ast::BinaryOp*>(tree))
            return partial_match(subtree, binary_tree->lhs) || 
                   partial_match(subtree, binary_tree->rhs);

        return false;
    }
}