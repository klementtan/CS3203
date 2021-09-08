// table.cpp
//
// stores implementation of result table for query

#include "pql/parser/ast.h"
#include "pql/eval/table.h"
#include "pql/exception.h"
#include "simple/ast.h"
#include <unordered_set>

namespace pql::eval::table
{
    extern const std::unordered_map<EntryType, std::string> EntryTypeString = {
        { EntryType::kNull, "Null" },
        { EntryType::kStmt, "Stmt" },
        { EntryType::kVar, "Var" },
        { EntryType::kProc, "Proc" },
        { EntryType::kConst, "Const" },
    };

    Entry::Entry() { }
    Entry::Entry(pql::ast::Declaration* declaration, std::string val)
    {
        this->m_declaration = declaration;
        this->m_val = val;
        switch(declaration->design_ent)
        {
            case ast::DESIGN_ENT::VARIABLE:
                this->m_type = EntryType::kVar;
                break;
            case ast::DESIGN_ENT::PROCEDURE:
                this->m_type = EntryType::kProc;
            case ast::DESIGN_ENT::CONSTANT:
                this->m_type = EntryType::kConst;
            default:
                throw exception::PqlException(
                    "pql::eval::table::Entry", "Entry for {} should be instantiated using stmt num instead of string");
        }
    }
    Entry::Entry(pql::ast::Declaration* declaration, simple::ast::StatementNum val)
    {
        if(declaration->design_ent == ast::DESIGN_ENT::VARIABLE ||
            declaration->design_ent == ast::DESIGN_ENT::PROCEDURE ||
            declaration->design_ent == ast::DESIGN_ENT::CONSTANT)
        {
            throw exception::PqlException(
                "pql::eval::table::Entry", "Entry for {} should be instantiated using string instead of stmt num");
        }
        this->m_declaration = declaration;
        this->m_type = EntryType::kStmt;
        this->m_stmt_num = val;
    }
    std::string Entry::getVal() const
    {
        if(this->m_type == EntryType::kStmt)
        {
            throw exception::PqlException("pql::eval::table::Entry", "Cannot getVal for statement entry");
        }
        return this->m_val;
    }
    simple::ast::StatementNum Entry::getStmtNum() const
    {
        if(this->m_type != EntryType::kStmt)
        {
            throw exception::PqlException("pql::eval::table::Entry", "Cannot getStmtNum for non-statement entry");
        }
        return this->m_stmt_num;
    }
    EntryType Entry::getType() const
    {
        return this->m_type;
    }
    std::string Entry::toString() const
    {
        return zpr::sprint("Entry(val:{}, stmt_num:{}, type:{}, declaration:{})", m_val, m_stmt_num,
            EntryTypeString.count(m_type) ? EntryTypeString.find(m_type)->second : "not found",
            m_declaration->toString());
    }
    bool Entry::operator==(const Entry& other) const
    {
        return this->m_type == other.getType() && this->m_stmt_num == other.getStmtNum() &&
               this->m_val == other.getVal() && this->m_declaration == other.getDeclaration();
    }
    bool Entry::operator!=(const Entry& other) const
    {
        return !(*this == other);
    }


    std::unordered_set<Entry> entry_set_intersect(
        const std::unordered_set<Entry>& a, const std::unordered_set<Entry>& b)
    {
        std::unordered_set<Entry> intersect;
        for(const Entry& entry_a : a)
        {
            if(b.count(entry_a))
                intersect.insert(entry_a);
        }
        return intersect;
    }
    Table::Table() { }


    void Table::upsertDomains(ast::Declaration* decl, std::unordered_set<Entry> entries)
    {
        m_domains[decl] = entries;
    }
    void Table::addJoin(const Entry& a, const Entry& b)
    {
        std::pair<ast::Declaration*, ast::Declaration*> key = order_join_key(a.getDeclaration(), b.getDeclaration());
        std::pair<Entry, Entry> val = order_join_val(a, b);
        auto it = m_joins.find(key);
        it->second.push_back(val);
    }

    std::vector<std::unordered_map<ast::Declaration*, Entry>> Table::getTablePerm() const
    {
        std::vector<std::unordered_map<ast::Declaration*, Entry>> ret;
        ret.push_back(std::unordered_map<ast::Declaration*, Entry>());

        // TODO: Use pruning to reduce search space
        for(auto decl_entries : m_domains)
        {
            ast::Declaration* decl = decl_entries.first;
            for(std::unordered_map<ast::Declaration*, Entry> combi : ret)
            {
                std::vector<std::unordered_map<ast::Declaration*, Entry>> new_ret;
                for(Entry entry : decl_entries.second)
                {
                    std::unordered_map<ast::Declaration*, Entry> new_combi(combi);
                    new_combi[decl] = entry;
                    new_ret.push_back(new_combi);
                }
                ret = new_ret;
            }
        }
        return ret;
    }
    void Table::clearDomains()
    {
        m_domains.clear();
    }

    std::unordered_set<Entry> Table::getDomain(ast::Declaration* decl) const
    {
        auto it = m_domains.find(decl);
        if(it == m_domains.end())
            return std::unordered_set<Entry>();
        return it->second;
    }

    std::list<std::string> Table::getResult(ast::Declaration* ret_decl)
    {
        std::list<std::string> result;
        std::vector<std::unordered_map<ast::Declaration*, Entry>> tables = getTablePerm();
        for(auto table : tables)
        {
            bool is_valid = true;
            for(auto join : m_joins)
            {
                auto [decl_ptr_a, decl_ptr_b] = join.first;
                assert(table.find(decl_ptr_a) != table.end());
                assert(table.find(decl_ptr_b) != table.end());
                Entry actual_entry_a = table.find(decl_ptr_a)->second;
                Entry actual_entry_b = table.find(decl_ptr_b)->second;
                bool has_valid_join = false;
                for(auto expected_entry_ab : join.second)
                {
                    // All joins should be have a valid declaration
                    Entry expected_entry_a = expected_entry_ab.first;
                    Entry expected_entry_b = expected_entry_ab.second;
                    if((actual_entry_a != expected_entry_a) || (actual_entry_a != expected_entry_b))
                    {
                        continue;
                    }
                    has_valid_join = true;
                    break;
                }
                if(!has_valid_join)
                {
                    is_valid = false;
                    break;
                }
            }
            if(is_valid)
            {
                Entry entry = table.find(ret_decl)->second;
                result.push_back(
                    entry.getType() == EntryType::kStmt ? std::to_string(entry.getStmtNum()) : entry.getVal());
            }
        }
        return result;
    }
}
