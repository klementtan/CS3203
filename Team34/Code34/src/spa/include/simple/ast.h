// ast.h
// contains definitions for all the abstract syntax tree nodes

#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>
#include <memory>

#include "util.h"

namespace simple::ast
{
    using StatementNum = size_t;

    struct Stmt;
    struct StmtList
    {
        std::vector<std::unique_ptr<Stmt>> statements {};

        const Stmt* parent_statement = nullptr;

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

        const StmtList* parent_list = 0;
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
        std::string value;
    };

    struct BinaryOp : Expr
    {
        virtual std::string toString() const override;

        std::unique_ptr<Expr> lhs {};
        std::unique_ptr<Expr> rhs {};

        // TODO: make this an enumeration
        std::string op;

        static bool isRelational(zst::str_view op);
        static bool isConditional(zst::str_view op);
    };

    struct UnaryOp : Expr
    {
        virtual std::string toString() const override;

        std::string op;
        std::unique_ptr<Expr> expr {};
    };

    struct IfStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::unique_ptr<Expr> condition {};

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

        std::unique_ptr<Expr> condition {};
        StmtList body;
    };

    struct AssignStmt : Stmt
    {
        virtual std::string toString(int nesting, bool compact = false) const override;

        std::string lhs;
        std::unique_ptr<Expr> rhs {};
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
        std::vector<std::unique_ptr<Procedure>> procedures;
    };

    bool exactMatch(const Expr* subtree, const Expr* tree);
    bool partialMatch(const Expr* subtree, const Expr* tree);
}
