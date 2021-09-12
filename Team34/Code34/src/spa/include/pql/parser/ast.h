// pqlast.h
// contains definitions for all abstract syntax tree nodes for pql

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "simple/ast.h"

namespace pql::ast
{
    struct Select;
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

    // Maps string representation of design ent to enum representation of it. ie: {"if": DESIGN_ENT::IF}
    extern const std::unordered_map<std::string, DESIGN_ENT> DESIGN_ENT_MAP;
    // Maps enum representation of design ent to string representation of it. ie: {DESIGN_ENT::IF: "if"}
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
        bool operator==(const Declaration& other) const;
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

        Declaration* declaration = nullptr;
    };

    /** Statement Reference using id(line number) of statement in program. */
    struct StmtId : StmtRef
    {
        virtual std::string toString() const override;

        size_t id = 0;
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

        Declaration* declaration = nullptr;
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

    /** Abstract class for Uses relationship condition. */
    struct Modifies : RelCond
    {
        virtual std::string toString() const = 0;
    };

    /** Represents `Modifies(EntRef, EntRef)` relationship condition. */
    struct ModifiesP : Modifies
    {
        virtual std::string toString() const override;

        EntRef* modifier = nullptr;
        EntRef* ent = nullptr;
    };

    /** Represents `Modifies(StmtRef, EntRef)` relationship condition. */
    struct ModifiesS : Modifies
    {
        virtual std::string toString() const override;

        StmtRef* modifier = nullptr;
        EntRef* ent = nullptr;
    };

    /** Abstract class for Uses relationship condition. */
    struct Uses : RelCond
    {
        virtual std::string toString() const = 0;
    };

    /** Represents `Uses(EntRef, EntRef)` relationship condition. */
    struct UsesP : Uses
    {
        virtual std::string toString() const override;

        EntRef* user = nullptr;
        EntRef* ent = nullptr;
    };

    /** Represents `Uses(StmtRef, EntRef)` relationship condition. */
    struct UsesS : Uses
    {
        virtual std::string toString() const override;

        StmtRef* user = nullptr;
        EntRef* ent = nullptr;
    };

    /** Represents `Parent(StmtRef, StmtRef)` relationship condition. */
    struct Parent : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* parent = nullptr;
        StmtRef* child = nullptr;
    };

    /** Represents `Parent*(StmtRef, StmtRef)` relationship condition. */
    struct ParentT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* ancestor = nullptr;
        StmtRef* descendant = nullptr;
    };

    /** Represents `Follows(StmtRef, StmtRef)` relationship condition. */
    struct Follows : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* directly_before = nullptr;
        StmtRef* directly_after = nullptr;
    };

    /** Represents `Follows*(StmtRef, StmtRef)` relationship condition. */
    struct FollowsT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef* before = nullptr;
        StmtRef* after = nullptr;
    };

    /** Expression Specification: pattern segment  of an assignment pattern. */
    struct ExprSpec
    {
        // Is prefixed by `_`
        bool any_before = false;
        // Is suffixed by `_`
        bool any_after = false;

        simple::ast::Expr* expr = nullptr;
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

        EntRef* ent = nullptr;
        Declaration* assignment_declaration = nullptr;
        ExprSpec* expr_spec = nullptr;
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
        SuchThatCl* such_that = nullptr;
        PatternCl* pattern = nullptr;
        Declaration* ent = nullptr;
        std::string toString() const;
    };

    struct Query
    {
        Select* select = nullptr;
        DeclarationList* declarations = nullptr;
        std::string toString() const;
    };
} // pql::ast

namespace std
{
    template <>
    struct hash<pql::ast::Declaration>
    {
        size_t operator()(pql::ast::Declaration& d) const
        {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + std::hash<std::string>()(d.name);
            res = res * 31 + std::hash<pql::ast::DESIGN_ENT>()(d.design_ent);
            return res;
        }
    };
}
