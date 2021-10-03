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
        PROCEDURE
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
    const std::unordered_set<DESIGN_ENT> kStmtDesignEntities = { DESIGN_ENT::STMT, DESIGN_ENT::READ, DESIGN_ENT::PRINT,
        DESIGN_ENT::CALL, DESIGN_ENT::WHILE, DESIGN_ENT::IF, DESIGN_ENT::ASSIGN };

    // Maps string representation of design ent to enum representation of it. ie: {"if": DESIGN_ENT::IF}
    extern const std::unordered_map<std::string, DESIGN_ENT> DESIGN_ENT_MAP;
    // Maps enum representation of design ent to string representation of it. ie: {DESIGN_ENT::IF: "if"}
    extern const std::unordered_map<DESIGN_ENT, std::string> INV_DESIGN_ENT_MAP;

    // Maps string representation of  attribute name to enum representation of it. ie: {"procName": AttrName::kProcName}
    extern const std::unordered_map<std::string, AttrName> AttrNameMap;
    // Maps enum representation of attribute name to string. ie: {AttrName::kProcName, "procName"}
    extern const std::unordered_map<AttrName, std::string> InvAttrNameMap;
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

    /** Represents `Calls(EntRef, EntRef)` */
    struct Calls : RelCond
    {
        virtual std::string toString() const override;

        EntRef caller {};
        EntRef proc {};
    };

    /** Represents `Calls*(EntRef, EntRef)` */
    struct CallsT : RelCond
    {
        virtual std::string toString() const override;

        EntRef caller {};
        EntRef proc {};
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

        virtual void evaluate(const pkb::ProgramKB* pkb, eval::table::Table* table) const = 0;
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

        std::string str() const;
        std::string number() const;
        AttrRef attrRef() const;
        Declaration* declaration() const;

        std::string toString() const;

    private:
        Type m_type = Type::Invalid;
        std::string _string_or_number {};
        AttrRef _attr_ref {};
        Declaration* _decl {};
    };

    struct WithCond
    {
        WithCondRef lhs {};
        WithCondRef rhs {};
        std::string toString() const;
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
        std::vector<std::unique_ptr<PatternCond>> patterns {};
        std::vector<std::unique_ptr<RelCond>> relations {};
        std::vector<std::unique_ptr<WithCond>> withs {};

        ResultCl result {};

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
