// pqlast.h
// contains definitions for all abstract syntax tree nodes for pql

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "ast.h"

namespace pql::ast
{
    struct Select;
    struct Clause;
    struct Declaration;

    enum class DESIGN_ENT
    {
        STMT,
        READ,
        PRINT,
        CALL,
        WHILE,
        IF,
        ASSIGN,
        VARIABLE,
        CONSTANT,
        PROCEDURE
    };

    // Maps string represnetation of design ent to enum represnetation of it. ie: {"if": DESIGN_ENT::IF}
    extern const std::unordered_map<std::string, DESIGN_ENT> DESIGN_ENT_MAP;
    // Maps enum represnetation of design ent to string represnetation of it. ie: {DESIGN_ENT::IF: "if"}
    extern const std::unordered_map<DESIGN_ENT, std::string> INV_DESIGN_ENT_MAP;

    /** List of design entity declaration. ie Represents [`assign a`, `print p`]. */
    struct DeclarationList
    {
        // Mapping of all declarations. <name, Declaration>
        std::unordered_map<std::string, Declaration*> declarations;

        std::string toString() const;
    };

    /** Design entity declaration. ie `assign a`. */
    struct Declaration
    {
        std::string toString() const;

        std::string name;
        DESIGN_ENT design_ent;
    };

    /** Abstract class for Statement Reference. */
    struct StmtRef
    {
        virtual ~StmtRef();
        virtual std::string toString() const = 0;
    };

    /** Statement Reference using previous pql Declaration. */
    struct DeclaredStmt : StmtRef
    {
        virtual std::string toString() const override;

        Declaration* declaration;
    };

    /** Statement Reference using id(line number) of statement in program. */
    struct StmtId : StmtRef
    {
        virtual std::string toString() const override;

        size_t id;
    };

    /** Statement Reference to all(`_`) statements. */
    struct AllStmt : StmtRef
    {
        virtual std::string toString() const override;
    };

    /** Abstract Reference for Entity Reference. */
    struct EntRef
    {
        virtual ~EntRef();
        virtual std::string toString() const = 0;
    };

    /** Entity Reference using previous pql Declaration. */
    struct DeclaredEnt : EntRef
    {
        virtual std::string toString() const override;

        Declaration* declaration;
    };

    /** Entity Reference using the string represnetation of variable name in program. */
    struct EntName : EntRef
    {
        virtual std::string toString() const override;

        std::string name;
    };

    /** Entity Reference to all(`_`) entities. */
    struct AllEnt : EntRef
    {
        virtual std::string toString() const override;
    };

    /** Abstract class for Relationship Conditions between Statements and Entities. */
    struct RelCond
    {
        virtual ~RelCond();
        virtual std::string toString() const = 0;
    };

    /** Represents `Modifies(StmtRef, EntRef)` relationship condition. */
    struct ModifiesS : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* modifier;
        EntRef* ent;
    };

    /** Represents `Uses(StmtRef, EntRef)` relationship condition. */
    struct UsesS : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* user;
        EntRef* ent;
    };

    /** Represents `Parent(StmtRef, StmtRef)` relationship condition. */
    struct Parent : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* parent;
        StmtRef* child;
    };

    /** Represents `Parent*(StmtRef, StmtRef)` relationship condition. */
    struct ParentT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* ancestor;
        StmtRef* descendant;
    };

    /** Represents `Follows(StmtRef, StmtRef)` relationship condition. */
    struct Follows : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* directly_before;
        StmtRef* directly_after;
    };

    /** Represents `Follows*(StmtRef, StmtRef)` relationship condition. */
    struct FollowsT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* before;
        StmtRef* after;
    };

    /** Expression Specification: pattern segment  of an assignment pattern. */
    struct ExprSpec
    {
        // Is prefixed by `_`
        bool any_before;
        // Is suffixed by `_`
        bool any_after;
        // The assignment expression on rhs that it should match
        // TODO: Refactor to use ast::Expr* instead of raw string for easier matching.
        std::string expr;
        std::string toString() const;
    };

    /** Condition for a pattern. */
    struct PatternCond
    {
        virtual ~PatternCond();
        virtual std::string toString() const = 0;
    };

    /** Assignment pattern condition. ie `assign a; Select a pattern a ("x",_);`*/
    struct AssignPatternCond : PatternCond
    {
        virtual std::string toString() const override;

        EntRef* ent;
        Declaration* assignment_declaration;
        ExprSpec* expr_spec;
    };

    /** Pattern Clause. */
    struct PatternCl
    {
        // Support multiple PatternCond for forward compatibility. Future iteration
        // requires ANDing multiple PatternCond
        std::vector<PatternCond*> pattern_conds;
        std::string toString() const;
    };

    /** SuchThat Clause. */
    struct SuchThatCl
    {
        // Support multiple RelCond for forward compatibility. Future iteration
        // requires ANDing multiple RelCond
        std::vector<RelCond*> rel_conds;
        std::string toString() const;
    };

    /** Select query. */
    struct Select
    {
        SuchThatCl* such_that;
        PatternCl* pattern;
        Declaration* ent;
        std::string toString() const;
    };

    struct Query
    {
        Select* select;
        DeclarationList* declarations;
        std::string toString() const;
    };
} // pqlast
