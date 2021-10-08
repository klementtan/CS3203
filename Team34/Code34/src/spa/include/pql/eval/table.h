// table.cpp
//
// stores declaration for table

#pragma once

#include "pql/parser/ast.h"
#include "simple/ast.h"
#include "pkb.h"
#include <unordered_set>
#include <list>

namespace pql::eval::table
{
    enum class EntryType
    {
        kNull = 0,
        kStmt,
        kVar,
        kProc,
        kConst,
    };

    class Entry
    {
    private:
        pql::ast::Declaration* m_declaration = nullptr;
        EntryType m_type = EntryType::kNull;
        std::string m_val {};
        simple::ast::StatementNum m_stmt_num { 0 };

    public:
        Entry();
        Entry(pql::ast::Declaration* declaration, const std::string& val);
        Entry(pql::ast::Declaration* declaration, const simple::ast::StatementNum& val);
        // Only use this for AttrRef as we cannot determine the entry type from the declaration
        Entry(pql::ast::Declaration* declaration, const std::string& val, EntryType type);
        [[nodiscard]] std::string getVal() const;
        [[nodiscard]] simple::ast::StatementNum getStmtNum() const;
        [[nodiscard]] EntryType getType() const;
        [[nodiscard]] ast::Declaration* getDeclaration() const;
        [[nodiscard]] std::string toString() const;
        bool operator==(const Entry& other) const;
        bool operator!=(const Entry& other) const;
    };
}

namespace std
{
    template <>
    struct hash<pql::eval::table::Entry>
    {
        size_t operator()(const pql::eval::table::Entry& e) const
        {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            if(e.getType() != pql::eval::table::EntryType::kStmt)
                res = res * 31 + std::hash<string>()(e.getVal());
            if(e.getType() == pql::eval::table::EntryType::kStmt)
                res = res * 31 + std::hash<simple::ast::StatementNum>()(e.getStmtNum());
            res = res * 31 + std::hash<pql::ast::Declaration>()(*e.getDeclaration());
            res = res * 31 + std::hash<pql::eval::table::EntryType>()(e.getType());
            return res;
        }
    };
    template <>
    struct hash<std::pair<pql::eval::table::Entry, pql::eval::table::Entry>>
    {
        size_t operator()(const std::pair<pql::eval::table::Entry, pql::eval::table::Entry>& p) const
        {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + std::hash<pql::eval::table::Entry>()(p.first);
            res = res * 31 + std::hash<pql::eval::table::Entry>()(p.second);
            return res;
        }
    };

    template <>
    struct hash<std::pair<pql::ast::Declaration*, pql::ast::Declaration*>>
    {
        size_t operator()(const std::pair<pql::ast::Declaration*, pql::ast::Declaration*>& p) const
        {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + std::hash<pql::ast::Declaration*>()(p.first);
            res = res * 31 + std::hash<pql::ast::Declaration*>()(p.second);
            return res;
        }
    };
}
namespace pql::eval::table
{
    using Domain = std::unordered_set<Entry>;
    using Row = std::unordered_map<ast::Declaration*, Entry>;

    Domain entry_set_intersect(const Domain& a, const Domain& b);

    // Class representing the dependency between two declaration in a clause.
    // For a table permutation to be valid, the value of m_decl_a,m_decl_b
    // must match one of the value in m_allowed_entries
    class Join
    {
    private:
        pql::ast::Declaration* m_decl_a;
        pql::ast::Declaration* m_decl_b;
        std::unordered_set<std::pair<Entry, Entry>> m_allowed_entries;

    public:
        Join() = default;
        Join(pql::ast::Declaration* decl_a, pql::ast::Declaration* decl_b,
            std::unordered_set<std::pair<Entry, Entry>> allowed_entries);
        void setAllowedEntries(const std::unordered_set<std::pair<Entry, Entry>>& allowed_entries);
        [[nodiscard]] pql::ast::Declaration* getDeclA() const;
        [[nodiscard]] pql::ast::Declaration* getDeclB() const;
        [[nodiscard]] std::unordered_set<std::pair<Entry, Entry>> getAllowedEntries() const;
        [[nodiscard]] std::unordered_set<std::pair<Entry, Entry>> getAllowedEntries(pql::ast::Declaration* decl) const;
        [[nodiscard]] std::string toString() const;
    };

    class Table
    {
    private:
        std::unordered_map<ast::Declaration*, Domain> m_domains;
        // Mapping of <declaration, declaration>: list of corresponding entry
        // All rows must equal to at least one of the entry pair
        std::vector<Join> m_joins;
        // All declaration involved in select query
        std::unordered_set<ast::Declaration*> m_select_decls;
        // Get all possible rows with decls as the column. The value of each entry in the
        // row will exist in the domain of declaration.
        [[nodiscard]] std::vector<Row> getRows(
            const std::vector<ast::Declaration*>& decls, const std::vector<Join>& joins) const;
        // Get the rows in candidate_rows that fulfill all of the joins
        [[nodiscard]] std::vector<Row> getValidRows(const std::vector<Row>& candidate_rows) const;
        // Get mapping of declaration to the join that is involved in.
        [[nodiscard]] std::unordered_map<ast::Declaration*, std::vector<Join>> getDeclJoins() const;
        [[nodiscard]] bool hasValidDomain() const;

    public:
        void putDomain(ast::Declaration* decl, const Domain& entries);
        void addSelectDecl(ast::Declaration* decl);
        static Entry extractAttr(const Entry& entry, const ast::AttrRef& attr_ref, const pkb::ProgramKB* pkb);
        Domain getDomain(ast::Declaration* decl) const;
        void addJoin(const Join& join);
        Table();
        ~Table();
        std::list<std::string> getResult(const ast::ResultCl& result, const pkb::ProgramKB* pkb);
        static std::list<std::string> getFailedResult(const ast::ResultCl& result);
        [[nodiscard]] std::string toString() const;
    };
}
