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

template <>
struct std::hash<pql::eval::table::Entry>
{
    size_t operator()(const pql::eval::table::Entry& e) const
    {
        if(e.getType() == pql::eval::table::EntryType::kStmt)
            return util::hash_combine(e.getStmtNum(), *e.getDeclaration(), e.getType());
        else
            return util::hash_combine(e.getVal(), *e.getDeclaration(), e.getType());
    }
};
template <>
struct std::hash<std::pair<pql::eval::table::Entry, pql::eval::table::Entry>>
{
    size_t operator()(const std::pair<pql::eval::table::Entry, pql::eval::table::Entry>& p) const
    {
        return util::hash_combine(p.first, p.second);
    }
};

template <>
struct std::hash<std::pair<pql::ast::Declaration*, pql::ast::Declaration*>>
{
    size_t operator()(const std::pair<pql::ast::Declaration*, pql::ast::Declaration*>& p) const
    {
        return util::hash_combine(*p.first, *p.second);
    }
};




namespace pql::eval::table
{
    using Domain = std::unordered_set<Entry>;
    using Row = std::unordered_map<ast::Declaration*, Entry>;

    // there isn't actually a need to restrict this to sets of Entry.
    template <typename T>
    std::unordered_set<T> setIntersction(const std::unordered_set<T>& a, const std::unordered_set<T>& b)
    {
        // always loop over the smaller one.
        if(b.size() > a.size())
            return setIntersction(b, a);

        std::unordered_set<T> ret {};
        for(const auto& entry : a)
        {
            if(b.count(entry) > 0)
                ret.insert(entry);
        }
        return ret;
    }

    constexpr auto entry_set_intersect = setIntersction<Entry>;

    // Class representing the dependency between two declaration in a clause.
    // For a table permutation to be valid, the value of m_decl_a,m_decl_b
    // must match one of the value in m_allowed_entries
    class Join
    {
    private:
        pql::ast::Declaration* m_decl_a;
        pql::ast::Declaration* m_decl_b;
        std::unordered_set<std::pair<Entry, Entry>> m_allowed_entries;
        int m_id;
        static int get_next_id();

    public:
        Join() = default;
        Join(pql::ast::Declaration* decl_a, pql::ast::Declaration* decl_b,
            std::unordered_set<std::pair<Entry, Entry>> allowed_entries);
        void setAllowedEntries(const std::unordered_set<std::pair<Entry, Entry>>& allowed_entries);
        [[nodiscard]] pql::ast::Declaration* getDeclA() const;
        [[nodiscard]] pql::ast::Declaration* getDeclB() const;
        [[nodiscard]] const std::unordered_set<std::pair<Entry, Entry>>& getAllowedEntries() const;
        [[nodiscard]] std::unordered_set<std::pair<Entry, Entry>>& getAllowedEntries();
        [[nodiscard]] bool isAllowedEntry(const std::pair<Entry, Entry>& entry) const;
        [[nodiscard]] std::string toString() const;
        [[nodiscard]] int getId() const;
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
        // Get mapping of declaration to the join that is involved in.
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
