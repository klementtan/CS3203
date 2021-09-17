// destructors.cpp

#include "simple/ast.h"

namespace simple::ast
{
    Stmt::~Stmt() { }
    Expr::~Expr() { }

    StmtList::~StmtList()
    {
        for(auto stmt : this->statements)
            delete stmt;
    }

    StmtList::StmtList(StmtList&& other)
    {
        this->parent_statement = other.parent_statement;
        this->statements = std::move(other.statements);
    }

    StmtList& StmtList::operator=(StmtList&& other)
    {
        if(this != &other)
        {
            this->parent_statement = other.parent_statement;
            this->statements = std::move(other.statements);
        }
        return *this;
    }

    BinaryOp::~BinaryOp()
    {
        delete this->lhs;
        delete this->rhs;
    }

    UnaryOp::~UnaryOp()
    {
        delete this->expr;
    }

    IfStmt::~IfStmt()
    {
        delete this->condition;
    }

    WhileLoop::~WhileLoop()
    {
        delete this->condition;
    }

    AssignStmt::~AssignStmt()
    {
        delete this->rhs;
    }

    Program::~Program()
    {
        for(auto proc : this->procedures)
            delete proc;
    }
}
