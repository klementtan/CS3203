// destructors.cpp

#include "pql/parser/ast.h"

namespace pql::ast
{
    DeclarationList::~DeclarationList()
    {
        for(const auto& [_, decl] : this->declarations)
            delete decl;
    }
}
