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
    std::string rowToString(const std::unordered_map<ast::Declaration*, Entry>& row);
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
                util::logfmt("pql::eval::table", "{} added to intersect", entry_a.toString());
                intersect.insert(entry_a);
            }
            else
            {
                util::logfmt("pql::eval::table", "{} does not exists in intersect", entry_a.toString());
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

    Table::Table() { }

    Table::~Table() { }


    void Table::upsertDomains(ast::Declaration* decl, const std::unordered_set<Entry>& entries)
    {
        util::logfmt("pql::eval::table", "Updating domain of {} with {} entries", decl->toString(), entries.size());
        m_domains[decl] = entries;
    }
    void Table::addJoin(const Join& join)
    {
        m_joins.push_back(join);
    }

    void Table::addSelectDecl(ast::Declaration* decl)
    {
        assert(decl);
        m_select_decls.insert(decl);
    }
    std::unordered_map<ast::Declaration*, std::vector<Join>> Table::getDeclJoins() const
    {
        std::unordered_map<ast::Declaration*, std::vector<Join>> decl_joins;
        for(const Join& join : m_joins)
        {
            decl_joins[join.getDeclA()].emplace_back(join);
            decl_joins[join.getDeclB()].emplace_back(join);
        }
        return decl_joins;
    }

    std::vector<Row> Table::getRows(const std::vector<ast::Declaration*>& decls, const std::vector<Join>& joins) const
    {
        std::unordered_map<ast::Declaration*, std::vector<Join>> decl_joins;
        for(const Join& join : joins)
        {
            decl_joins[join.getDeclA()].push_back(join);
            decl_joins[join.getDeclB()].push_back(join);
        }
        std::vector<std::unordered_map<ast::Declaration*, Entry>> rows;
        rows.emplace_back();

        for(ast::Declaration* decl : decls)
        {
            util::logfmt("pql::eval::table", "Adding {} to rows", decl->toString());
            Domain entries = Table::getDomain(decl);
            std::vector<Row> new_rows;
            for(const Entry& entry : entries)
            {
                for(const Row& row : rows)
                {
                    std::unordered_map<ast::Declaration*, Entry> new_row(row);
                    new_row[decl] = entry;
                    new_rows.push_back(new_row);
                }
            }
            std::vector<Row> sanitized_new_rows;
            // TODO: Use dfs on Joins to reduce search space instead of checking against every join for each permutation
            for(const Row& row : new_rows)
            {
                bool is_valid = true;
                if(decl_joins.count(decl) == 0)
                {
                    util::logfmt("pql::eval::table", "Cannot prune as {} has not join dependency", decl->toString());
                    continue;
                }
                for(const Join& join : decl_joins.find(decl)->second)
                {
                    ast::Declaration* decl_ptr_a = join.getDeclA();
                    ast::Declaration* decl_ptr_b = join.getDeclB();
                    if(row.count(decl_ptr_a) == 0 || row.count(decl_ptr_b) == 0)
                    {
                        continue;
                    }
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
                    sanitized_new_rows.push_back(row);
                }
            }
            rows = sanitized_new_rows;
        }
        return rows;
    }
    std::vector<Row> Table::getValidRows(const std::vector<Row>& candidate_rows) const
    {
        std::vector<Row> valid_rows;
        for(auto row : candidate_rows)
        {
            util::logfmt("pql::eval::row", "Checking if row fulfill all {} join condition: {}", m_joins.size(),
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
                    util::logfmt("pql::eval::row", "Found a valid Join({}={}, {}={}) ", decl_ptr_a->toString(),
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
                valid_rows.push_back(row);
            }
        }
        return valid_rows;
    }

    std::unordered_set<Entry> Table::getDomain(ast::Declaration* decl) const
    {
        auto it = m_domains.find(decl);
        if(it == m_domains.end())
            return std::unordered_set<Entry>();
        return it->second;
    }

    std::string rowToString(const Row& row)
    {
        std::string ret { "Row:[\n" };
        for(auto [decl_ptr, entry] : row)
        {
            ret += zpr::sprint("{}={}\n", decl_ptr->toString(), entry.toString());
        }
        ret += "]\n";
        return ret;
    }

    bool Table::hasValidDomain() const
    {
        for(ast::Declaration* decl : m_select_decls)
        {
            util::logfmt("pql::eval::table", "Checking if {} has non empty domain", decl->toString());
            std::unordered_set<Entry> domain = getDomain(decl);
            // All declarations should have at least one entry in domain
            if(domain.empty())
            {
                util::logfmt("pql::eval", "{} has empty domain", decl->toString());
                return false;
            }
        }
        return true;
    }


    std::list<std::string> Table::getResult(ast::Declaration* ret_decl)
    {
        std::unordered_set<Entry> result_entry;
        std::unordered_map<ast::Declaration*, std::vector<Join>> decl_joins = getDeclJoins();

        // We only need to permutate on decl involded in joins
        std::vector<ast::Declaration*> join_decls;
        for(auto [decl, joins] : decl_joins)
        {
            join_decls.push_back(decl);
        }

        // All domain involved in query should have non-empty domain
        if(!hasValidDomain())
            return {};

        std::vector<Row> candidate_rows = getRows(join_decls, m_joins);
        std::vector<Row> valid_rows = getValidRows(candidate_rows);

        // There should exist a valid assignment that satisfies all joins even if ret declaration is not
        // involved in any clause
        if(valid_rows.empty())
            return {};

        if(decl_joins.count(ret_decl))
        {
            for(const Row& row : valid_rows)
            {
                auto it = row.find(ret_decl);
                // Return decl shold be a column in valid rows;
                assert(it != row.end());
                Entry ret_decl_entry = it->second;
                result_entry.insert(ret_decl_entry);
            }
        }
        else
        {
            result_entry = Table::getDomain(ret_decl);
        }

        std::list<std::string> result;

        for(const Entry& entry : result_entry)
        {
            result.push_back(entry.getType() == EntryType::kStmt ? std::to_string(entry.getStmtNum()) : entry.getVal());
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
