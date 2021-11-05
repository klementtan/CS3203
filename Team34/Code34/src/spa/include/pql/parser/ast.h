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
        INVALID,

        STMT,
        READ,
        PRINT,
        CALL,
        WHILE,
        IF,
        ASSIGN,
        VARIABLE,
        CONSTANT,
        PROCEDURE,
        PROG_LINE,
    };

    enum class AttrName
    {
        kInvalid,
        kProcName,
        kVarName,
        kValue,
        kStmtNum
    };

    // Set of all statement design entities
    const std::unordered_set<DESIGN_ENT>& getStmtDesignEntities();

    // Maps string representation of design ent to enum representation of it. ie: {"if": DESIGN_ENT::IF}
    const std::unordered_map<std::string, DESIGN_ENT>& getDesignEntityMap();

    // Maps enum representation of design ent to string representation of it. ie: {DESIGN_ENT::IF: "if"}
    const std::unordered_map<DESIGN_ENT, std::string>& getInverseDesignEntityMap();

    // Maps string representation of  attribute name to enum representation of it. ie: {"procName": AttrName::kProcName}
    const std::unordered_map<std::string, AttrName>& getAttrNameMap();

    // Maps enum representation of attribute name to string. ie: {AttrName::kProcName, "procName"}
    const std::unordered_map<AttrName, std::string>& getInverseAttrNameMap();

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

    private:
        Type ref_type = Type::Invalid;
        Declaration* _declaration {};
        simple::ast::StatementNum _id {};
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

        std::string toString() const;
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

    private:
        Type ref_type = Type::Invalid;
        Declaration* _declaration {};
        std::string _name {};
    };


    // Represents the reference to a attribute of a synonym
    struct AttrRef
    {
        Declaration* decl = nullptr;
        AttrName attr_name = AttrName::kInvalid;

        std::string toString() const;
    };

    // Represents the element to return in select clause
    /** Abstract class for Statement Reference. */
    struct Elem
    {
        enum class Type
        {
            Invalid,
            Declaration,
            AttrRef,
        };

        std::string toString() const;


        Declaration* declaration() const;
        AttrRef attrRef() const;

        inline bool isAttrRef() const
        {
            return ref_type == Type::AttrRef;
        }
        inline bool isDeclaration() const
        {
            return ref_type == Type::Declaration;
        }

        static Elem ofDeclaration(Declaration* decl);
        static Elem ofAttrRef(AttrRef AttrRef);

    private:
        Type ref_type = Type::Invalid;
        Declaration* _declaration {};
        AttrRef _attr_ref {};
    };

    // the (abstract) base class for all PQL clauses -- relational, with, and pattern.
    struct Clause
    {
        virtual ~Clause();
        virtual std::string toString() const = 0;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const = 0;
    };

    /** Abstract class for Relationship Conditions between Statements and Entities. */
    struct RelCond : Clause
    {
    };

    /** Represents `Modifies(EntRef, EntRef)` relationship condition. */
    struct ModifiesP : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        EntRef modifier {};
        EntRef ent {};
    };

    /** Represents `Modifies(StmtRef, EntRef)` relationship condition. */
    struct ModifiesS : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef modifier {};
        EntRef ent {};
    };

    /** Represents `Uses(EntRef, EntRef)` relationship condition. */
    struct UsesP : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        EntRef user {};
        EntRef ent {};
    };

    /** Represents `Uses(StmtRef, EntRef)` relationship condition. */
    struct UsesS : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef user {};
        EntRef ent {};
    };

    /** Represents `Parent(StmtRef, StmtRef)` relationship condition. */
    struct Parent : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef parent {};
        StmtRef child {};
    };

    /** Represents `Parent*(StmtRef, StmtRef)` relationship condition. */
    struct ParentT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef ancestor {};
        StmtRef descendant {};
    };

    /** Represents `Follows(StmtRef, StmtRef)` relationship condition. */
    struct Follows : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef directly_before {};
        StmtRef directly_after {};
    };

    /** Represents `Follows*(StmtRef, StmtRef)` relationship condition. */
    struct FollowsT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef before {};
        StmtRef after {};
    };

    /** Represents `Calls(EntRef, EntRef)` */
    struct Calls : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        EntRef caller {};
        EntRef proc {};
    };

    /** Represents `Calls*(EntRef, EntRef)` */
    struct CallsT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        EntRef caller {};
        EntRef proc {};
    };

    struct Next : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct NextT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct Affects : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct AffectsT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct NextBip : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct NextBipT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct AffectsBip : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
    };

    struct AffectsBipT : RelCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        StmtRef first {};
        StmtRef second {};
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
    struct PatternCond : Clause
    {
    };

    /** Assignment pattern condition. ie `assign a; Select a pattern a ("x",_);`*/
    struct AssignPatternCond : PatternCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        Declaration* assignment_declaration = nullptr;

        EntRef ent {};
        ExprSpec expr_spec {};
    };

    /** If pattern condition. ie `if i; Select i pattern i ("x", _, _);` */
    struct IfPatternCond : PatternCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        Declaration* if_declaration = nullptr;
        EntRef ent {};
    };

    /** While pattern condition. ie `while w; Select w pattern w ("x", _);` */
    struct WhilePatternCond : PatternCond
    {
        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;

        Declaration* while_declaration = nullptr;
        EntRef ent {};
    };

    // one side of a with condition
    struct WithCondRef
    {
        enum class Type
        {
            Invalid,
            Declaration,
            AttrRef,
            Number,
            String,
        };


        inline bool isString() const
        {
            return m_type == Type::String;
        }
        inline bool isNumber() const
        {
            return m_type == Type::Number;
        }
        inline bool isAttrRef() const
        {
            return m_type == Type::AttrRef;
        }
        inline bool isDeclaration() const
        {
            return m_type == Type::Declaration;
        }

        static WithCondRef ofString(std::string s);
        static WithCondRef ofNumber(std::string i);
        static WithCondRef ofAttrRef(AttrRef a);
        static WithCondRef ofDeclaration(Declaration* d);

        std::string stringOrNumber() const;
        AttrRef attrRef() const;
        Declaration* declaration() const;

        std::string toString() const;

    private:
        Type m_type = Type::Invalid;
        std::string _string_or_number {};
        AttrRef _attr_ref {};
        Declaration* _decl {};
    };

    struct WithCond : Clause
    {
        WithCondRef lhs {};
        WithCondRef rhs {};

        virtual std::string toString() const override;
        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const override;
    };

    struct ResultCl
    {
        enum class Type
        {
            Invalid,
            Bool,
            Tuple,
        };

        Type type;
        std::vector<Elem> _tuple;

        inline bool isBool() const
        {
            return type == Type::Bool;
        }

        inline bool isTuple() const
        {
            return type == Type::Tuple;
        }

        static ResultCl ofBool();

        static ResultCl ofTuple(const std::vector<Elem>& tuple);

        std::vector<Elem> tuple() const;

        std::string toString() const;
    };

    /** Select query. */
    struct Select
    {
        ResultCl result {};
        std::vector<std::unique_ptr<Clause>> clauses {};

        std::string toString() const;
    };

    struct Query
    {
        Select select {};
        DeclarationList declarations {};

        bool is_semantically_invalid = false;

        std::string toString() const;

        inline void setInvalid()
        {
            is_semantically_invalid = true;
        }

        inline bool isInvalid() const
        {
            return is_semantically_invalid;
        }

        inline bool isValid() const
        {
            return !is_semantically_invalid;
        }
    };

} // pql::ast


template <>
struct std::hash<pql::ast::Declaration>
{
    size_t operator()(const pql::ast::Declaration& d) const
    {
        return util::hash_combine(d.name, d.design_ent);
    }
};
