// table.cpp
//
// stores declaration for table

#pragma once

#include "pql/parser/ast.h"
#include "simple/ast.h"
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
    std::unordered_set<Entry> entry_set_intersect(
        const std::unordered_set<Entry>& a, const std::unordered_set<Entry>& b);

    class Table
    {
    private:
        std::unordered_map<ast::Declaration*, std::unordered_set<Entry>> m_domains;
        // Mapping of <declaration, declaration>: list of corresponding entry
        // All rows must equal to at least one of the entry pair
        std::unordered_map<std::pair<ast::Declaration*, ast::Declaration*>, std::vector<std::pair<Entry, Entry>>>
            m_joins;
        static std::pair<ast::Declaration*, ast::Declaration*> order_join_key(ast::Declaration* a, ast::Declaration* b)
        {
            if(a->name < b->name)
            {
                return std::make_pair(a, b);
            }
            else
            {
                return std::make_pair(b, a);
            }
        }
        static std::pair<Entry, Entry> order_join_val(const Entry& a, const Entry& b)
        {
            if(a.getDeclaration()->name < b.getDeclaration()->name)
            {
                return std::make_pair(a, b);
            }
            else
            {
                return std::make_pair(b, a);
            }
        }

        [[nodiscard]] std::vector<std::unordered_map<ast::Declaration*, Entry>> getTablePerm() const;

    public:
        void upsertDomains(ast::Declaration* decl, const std::unordered_set<Entry>& entries);
        std::unordered_set<Entry> getDomain(ast::Declaration* decl) const;
        void addJoin(const Entry& a, const Entry& b);
        Table();
        std::list<std::string> getResult(ast::Declaration* ret_decls);
        [[nodiscard]] std::string toString() const;
    };
}