// pqlast.h
// contains definitions for all abstract syntax tree nodes for pql

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "simple/ast.h"

namespace pkb
{
    struct ProgramKB;
}

namespace pql::eval::table
{
    class Table;
}

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

    // Set of all statement design entities
    const std::unordered_set<DESIGN_ENT> kStmtDesignEntities = { DESIGN_ENT::STMT, DESIGN_ENT::READ, DESIGN_ENT::PRINT,
        DESIGN_ENT::CALL, DESIGN_ENT::WHILE, DESIGN_ENT::IF, DESIGN_ENT::ASSIGN };

    // Maps string representation of design ent to enum representation of it. ie: {"if": DESIGN_ENT::IF}
    extern const std::unordered_map<std::string, DESIGN_ENT> DESIGN_ENT_MAP;
    // Maps enum representation of design ent to string representation of it. ie: {DESIGN_ENT::IF: "if"}
    extern const std::unordered_map<DESIGN_ENT, std::string> INV_DESIGN_ENT_MAP;

    /** List of design entity declaration. ie Represents [`assign a`, `print p`]. */
    struct DeclarationList
    {
        DeclarationList() = default;
        ~DeclarationList();

        DeclarationList(const DeclarationList&) = delete;
        DeclarationList& operator=(const DeclarationList&) = delete;

        DeclarationList(DeclarationList&&) = default;
        DeclarationList& operator=(DeclarationList&&) = default;


        std::string toString() const;
        bool hasDeclaration(const std::string& name) const;
        Declaration* getDeclaration(const std::string& name) const;
        const std::unordered_map<std::string, Declaration*>& getAllDeclarations() const;

        void addDeclaration(std::string name, Declaration* decl);

    private:
        std::unordered_map<std::string, Declaration*> declarations {};
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
        enum class Type
        {
            Invalid,
            Declaration,
            StmtId,
            Wildcard
        };

        std::string toString() const;

        Type ref_type {};
        union {
            Declaration* declaration;
            simple::ast::StatementNum id;
        };

        inline bool isWildcard() const { return ref_type == Type::Wildcard; }
        inline bool isStatementId() const { return ref_type == Type::StmtId; }
        inline bool isDeclaration() const { return ref_type == Type::Declaration; }

        static StmtRef ofWildcard();
        static StmtRef ofDeclaration(Declaration* decl);
        static StmtRef ofStatementId(simple::ast::StatementNum id);
    };

    // /** Statement Reference using previous pql Declaration. */
    // struct DeclaredStmt : StmtRef
    // {
    //     virtual std::string toString() const override;

    // };

    // /** Statement Reference using id(line number) of statement in program. */
    // struct StmtId : StmtRef
    // {
    //     virtual std::string toString() const override;

    //      // = 0;
    // };

    // /** Statement Reference to all(`_`) statements. */
    // struct AllStmt : StmtRef
    // {
    //     virtual std::string toString() const override;
    // };

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

        StmtRef modifier {};
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

        StmtRef user {};
        EntRef* ent = nullptr;
    };

    /** Represents `Parent(StmtRef, StmtRef)` relationship condition. */
    struct Parent : RelCond
    {
        virtual std::string toString() const override;

        StmtRef parent {};
        StmtRef child {};
    };

    /** Represents `Parent*(StmtRef, StmtRef)` relationship condition. */
    struct ParentT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef ancestor {};
        StmtRef descendant {};
    };

    /** Represents `Follows(StmtRef, StmtRef)` relationship condition. */
    struct Follows : RelCond
    {
        virtual std::string toString() const override;

        StmtRef directly_before {};
        StmtRef directly_after {};
    };

    /** Represents `Follows*(StmtRef, StmtRef)` relationship condition. */
    struct FollowsT : RelCond
    {
        virtual std::string toString() const override;

        StmtRef before {};
        StmtRef after {};
    };

    /** Expression Specification: pattern segment  of an assignment pattern. */
    struct ExprSpec
    {
        // whether this is surrounded by '_'s
        bool is_subexpr = false;
        std::unique_ptr<simple::ast::Expr> expr {};

        std::string toString() const;
    };

    /** Condition for a pattern. */
    struct PatternCond
    {
        virtual ~PatternCond();
        virtual std::string toString() const = 0;

        virtual void evaluate(pkb::ProgramKB* pkb, eval::table::Table* table) const = 0;
    };

    /** Assignment pattern condition. ie `assign a; Select a pattern a ("x",_);`*/
    struct AssignPatternCond : PatternCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        Declaration* assignment_declaration = nullptr;

        EntRef* ent {};
        ExprSpec expr_spec {};
    };

    /** Pattern Clause. */
    struct PatternCl
    {
        // Support multiple PatternCond for forward compatibility. Future iteration
        // requires ANDing multiple PatternCond
        std::vector<std::unique_ptr<PatternCond>> pattern_conds;
        std::string toString() const;
    };

    /** SuchThat Clause. */
    struct SuchThatCl
    {
        // Support multiple RelCond for forward compatibility. Future iteration
        // requires ANDing multiple RelCond
        std::vector<std::unique_ptr<RelCond>> rel_conds;
        std::string toString() const;
    };

    /** Select query. */
    struct Select
    {
        std::optional<SuchThatCl> such_that {};
        std::optional<PatternCl> pattern {};
        Declaration* ent = nullptr;

        std::string toString() const;
    };

    struct Query
    {
        Select select {};
        DeclarationList declarations {};

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
