// ast.h
// contains definitions for all the abstract syntax tree nodes

#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>

#include "util.h"

namespace simple::ast
{
    using StatementNum = size_t;

    struct Stmt;
    struct StmtList
    {
        std::vector<Stmt*> statements;

        Stmt* parent_statement = nullptr;

        std::string toString(int nesting, bool compact = false) const;
    };

    struct Expr
    {
        virtual ~Expr();
        virtual std::string toString() const = 0;
    };

    struct Stmt
    {
        virtual ~Stmt();
        virtual std::string toString(int nesting, bool compact = false) const = 0;

        StmtList* parent_list = 0;
        StatementNum id = 0;
    };

    struct VarRef : Expr
    {
        virtual std::string toString() const override;
        std::string name;
    };

    struct Constant : Expr
    {
        virtual std::string toString() const override;
        int value = 0;
    };

    struct BinaryOp : Expr
    {
        virtual std::string toString() const override;

        Expr* lhs = 0;
        Expr* rhs = 0;

        // TODO: make this an enumeration
        std::string op;

        static bool isRelational(zst::str_view op);
        static bool isConditional(zst::str_view op);
    };

    struct UnaryOp : Expr
    {
        virtual std::string toString() const override;

        std::string op;
        Expr* expr = 0;
    };

    struct IfStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        Expr* condition = 0;

        StmtList true_case;
        StmtList false_case;
    };

    struct ProcCall : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::string proc_name;
    };

    struct WhileLoop : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        Expr* condition = 0;
        StmtList body;
    };

    struct AssignStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::string lhs;
        Expr* rhs = 0;
    };

    struct ReadStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::string var_name;
    };

    struct PrintStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::string var_name;
    };

    struct Procedure
    {
        std::string toString(bool compact = false) const;
        std::string name;
        StmtList body;
    };

    struct Program
    {
        std::string toString(bool compact = false) const;
        std::vector<Procedure*> procedures;
    };

    bool exactMatch(Expr* subtree, Expr* tree);
    bool partialMatch(Expr* subtree, Expr* tree);
}
