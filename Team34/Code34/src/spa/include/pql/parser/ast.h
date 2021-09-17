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

        Declaration* addDeclaration(const std::string& name, DESIGN_ENT design_ent);

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
        union
        {
            Declaration* _declaration;
            simple::ast::StatementNum _id;
        };

        Declaration* declaration() const;
        simple::ast::StatementNum id() const;

        inline bool isWildcard() const
        {
            return ref_type == Type::Wildcard;
        }
        inline bool isStatementId() const
        {
            return ref_type == Type::StmtId;
        }
        inline bool isDeclaration() const
        {
            return ref_type == Type::Declaration;
        }

        static StmtRef ofWildcard();
        static StmtRef ofDeclaration(Declaration* decl);
        static StmtRef ofStatementId(simple::ast::StatementNum id);
    };

    struct EntRef
    {
        enum class Type
        {
            Invalid,
            Declaration,
            Name,
            Wildcard
        };

        // needs the rule of 5 or whatever cos of std::string in the union
        EntRef() = default;
        ~EntRef();

        EntRef(const EntRef&);
        EntRef& operator=(const EntRef&);

        EntRef(EntRef&&);
        EntRef& operator=(EntRef&&);

        std::string toString() const;

        Type ref_type {};
        union
        {
            Declaration* _declaration;
            std::string _name {};
        };

        Declaration* declaration() const;
        std::string name() const;

        inline bool isName() const
        {
            return ref_type == Type::Name;
        }
        inline bool isWildcard() const
        {
            return ref_type == Type::Wildcard;
        }
        inline bool isDeclaration() const
        {
            return ref_type == Type::Declaration;
        }

        static EntRef ofWildcard();
        static EntRef ofName(std::string name);
        static EntRef ofDeclaration(Declaration* decl);
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

        EntRef modifier {};
        EntRef ent {};
    };

    /** Represents `Modifies(StmtRef, EntRef)` relationship condition. */
    struct ModifiesS : Modifies
    {
        virtual std::string toString() const override;

        StmtRef modifier {};
        EntRef ent {};
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

        EntRef user {};
        EntRef ent {};
    };

    /** Represents `Uses(StmtRef, EntRef)` relationship condition. */
    struct UsesS : Uses
    {
        virtual std::string toString() const override;

        StmtRef user {};
        EntRef ent {};
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

        EntRef ent {};
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
