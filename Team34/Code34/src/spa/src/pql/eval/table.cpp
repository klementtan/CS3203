// table.cpp
//
// stores implementation of result table for query

#include <cassert>
#include <unordered_set>

#include "zpr.h"

#include "exceptions.h"
#include "simple/ast.h"
#include "pql/parser/ast.h"
#include "pql/eval/table.h"

namespace pql::eval::table
{
    extern const std::unordered_map<EntryType, std::string> EntryTypeString = {
        { EntryType::kNull, "Null" },
        { EntryType::kStmt, "Stmt" },
        { EntryType::kVar, "Var" },
        { EntryType::kProc, "Proc" },
        { EntryType::kConst, "Const" },
    };

    Entry::Entry() = default;
    Entry::Entry(pql::ast::Declaration* declaration, const std::string& val)
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
                break;
            case ast::DESIGN_ENT::CONSTANT:
                this->m_type = EntryType::kConst;
                break;
            default:
                throw util::PqlException(
                    "pql::eval::table::Entry", "Entry for {} should be instantiated using stmt num instead of string");
        }
    }
    Entry::Entry(pql::ast::Declaration* declaration, const simple::ast::StatementNum& val)
    {
        if(declaration->design_ent == ast::DESIGN_ENT::VARIABLE ||
            declaration->design_ent == ast::DESIGN_ENT::PROCEDURE ||
            declaration->design_ent == ast::DESIGN_ENT::CONSTANT)
        {
            throw util::PqlException(
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
            throw util::PqlException("pql::eval::table::Entry", "Cannot getVal for statement entry");
        }
        return this->m_val;
    }
    simple::ast::StatementNum Entry::getStmtNum() const
    {
        if(this->m_type != EntryType::kStmt)
        {
            throw util::PqlException("pql::eval::table::Entry", "Cannot getStmtNum for non-statement entry");
        }
        return this->m_stmt_num;
    }
    EntryType Entry::getType() const
    {
        return this->m_type;
    }
    ast::Declaration* Entry::getDeclaration() const
    {
        return this->m_declaration;
    }
    std::string Entry::toString() const
    {
        return zpr::sprint("Entry(val:{}, stmt_num:{}, type:{}, declaration:{})", m_val, m_stmt_num,
            EntryTypeString.count(m_type) ? EntryTypeString.find(m_type)->second : "not found",
            m_declaration->toString());
    }
    bool Entry::operator==(const Entry& other) const
    {
        return this->m_type == other.getType() && this->m_declaration == other.getDeclaration() &&
               ((this->m_type == EntryType::kStmt && this->m_stmt_num == other.getStmtNum()) ||
                   (this->m_type != EntryType::kStmt && this->m_val == other.getVal()));
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
            {
                util::log("pql::eval::table", "{} added to intersect", entry_a.toString());
                intersect.insert(entry_a);
            }
            else
            {
                util::log("pql::eval::table", "{} does not exists in intersect", entry_a.toString());
            }
        }
        return intersect;
    }

    Join::Join(pql::ast::Declaration* decl_a, pql::ast::Declaration* decl_b,
        std::unordered_set<std::pair<Entry, Entry>> allowed_entries)
    {
        if(!decl_a)
        {
            throw util::PqlException("pql::eval::table", "Join cannot be instantiated with decl_a=nullptr");
        }
        if(!decl_b)
        {
            throw util::PqlException("pql::eval::table", "Join cannot be instantiated with decl_b=nullptr");
        }
        this->m_decl_a = decl_a;
        this->m_decl_b = decl_b;
        this->m_allowed_entries = allowed_entries;
    }

    pql::ast::Declaration* Join::getDeclA() const
    {
        return this->m_decl_a;
    }

    pql::ast::Declaration* Join::getDeclB() const
    {
        return this->m_decl_b;
    }

    std::unordered_set<std::pair<Entry, Entry>> Join::getAllowedEntries() const
    {
        return this->m_allowed_entries;
    }

    std::string Join::toString() const
    {
        std::string ret = zpr::sprint("Join(m_decl_a={}, m_decl_b={}", m_decl_a->toString(), m_decl_b->toString());
        ret += "\n\tm_allowed_entries=[\n";
        for(auto [entry_a, entry_b] : m_allowed_entries)
        {
            ret += zpr::sprint("(\t\tdecl_a={}, decl_b={})\n", entry_a.toString(), entry_b.toString());
        }
        ret += "])";
        return ret;
    }

    Table::Table() = default;


    void Table::upsertDomains(ast::Declaration* decl, const std::unordered_set<Entry>& entries)
    {
        m_domains[decl] = entries;
    }
    void Table::addJoin(const Join& join)
    {
        m_joins.push_back(join);
    }

    std::vector<std::unordered_map<ast::Declaration*, Entry>> Table::getRows() const
    {
        std::vector<std::unordered_map<ast::Declaration*, Entry>> ret;
        ret.emplace_back();

        // TODO: Use pruning to reduce search space
        for(auto [decl, entries] : m_domains)
        {
            std::vector<std::unordered_map<ast::Declaration*, Entry>> new_ret;
            for(const Entry& entry : entries)
            {
                for(const std::unordered_map<ast::Declaration*, Entry>& table_perm : ret)
                {
                    util::log("pql::eval::table", "Adding {} for each table perm", entry.toString());
                    std::unordered_map<ast::Declaration*, Entry> new_table_perm(table_perm);
                    new_table_perm[decl] = entry;
                    new_ret.push_back(new_table_perm);
                }
            }
            ret = new_ret;
        }
        return ret;
    }

    std::unordered_set<Entry> Table::getDomain(ast::Declaration* decl) const
    {
        auto it = m_domains.find(decl);
        if(it == m_domains.end())
            return std::unordered_set<Entry>();
        return it->second;
    }

    std::string rowToString(const std::unordered_map<ast::Declaration*, Entry>& row)
    {
        std::string ret { "Row:[\n" };
        for(auto [decl_ptr, entry] : row)
        {
            ret += zpr::sprint("{}={}\n", decl_ptr->toString(),entry.toString());
        }
        ret += "]\n";
        return ret;
    }

    std::list<std::string> Table::getResult(ast::Declaration* ret_decl)
    {
        std::list<std::string> result;
        std::vector<std::unordered_map<ast::Declaration*, Entry>> rows = getRows();
        for(auto row : rows)
        {
            util::log("pql::eval::row", "Checking if row fulfill all {} join condition: {}", m_joins.size(),
                      rowToString(row));
            bool is_valid = true;
            for(const Join& join : m_joins)
            {
                ast::Declaration* decl_ptr_a = join.getDeclA();
                ast::Declaration* decl_ptr_b = join.getDeclB();
                assert(row.find(decl_ptr_a) != row.end());
                assert(row.find(decl_ptr_b) != row.end());
                Entry actual_entry_a = row.find(decl_ptr_a)->second;
                Entry actual_entry_b = row.find(decl_ptr_b)->second;
                bool has_valid_join = false;
                for(const auto& expected_entry_ab : join.getAllowedEntries())
                {
                    // All joins should be have a valid declaration
                    Entry expected_entry_a = expected_entry_ab.first;
                    Entry expected_entry_b = expected_entry_ab.second;
                    if((actual_entry_a != expected_entry_a) || (actual_entry_b != expected_entry_b))
                    {
                        continue;
                    }
                    assert(actual_entry_a == expected_entry_a);
                    assert(actual_entry_b == expected_entry_b);
                    util::log("pql::eval::row", "Found a valid Join({}={}, {}={}) ", decl_ptr_a->toString(),
                        actual_entry_a.toString(), decl_ptr_b->toString(), actual_entry_b.toString());
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
                util::log("pql::eval::row", "Found valid row {}", rowToString(row));
                Entry entry = row.find(ret_decl)->second;
                result.push_back(
                    entry.getType() == EntryType::kStmt ? std::to_string(entry.getStmtNum()) : entry.getVal());
            }
        }
        return result;
    }
    std::string Table::toString() const
    {
        std::string ret = "Table(\n";
        ret += "\tm_domains[\n";
        for(auto [decl_ptr, entries] : m_domains)
        {
            ret += zpr::sprint("\t\t{}:[", decl_ptr->toString());
            for(const auto& entry : entries)
            {
                ret += zpr::sprint("{},", entry.toString());
            }
            ret += "],\n";
        }
        ret += "\t]\n";
        ret += "\tm_joins[\n";
        for(const Join& join : m_joins)
        {
            ret += zpr::sprint("\t\t{}\n", join.toString());
        }
        ret += "\t]\n";
        ret += ")\n";
        return ret;
    }
}
