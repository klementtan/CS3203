// table.cpp
//
// stores implementation of result table for query

#include <unordered_set>
#include <numeric>

#include "zpr.h"
#include "timer.h"
#include "exceptions.h"
#include "simple/ast.h"
#include "pkb.h"
#include "pql/parser/ast.h"
#include "pql/eval/table.h"
#include "pql/eval/solver.h"

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
    Entry::Entry(const pql::ast::Declaration* declaration, const std::string& val)
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
                throw util::PqlException("pql::eval::table::Entry",
                    "Entry for {} should be instantiated using stmt num instead of string", declaration->toString());
        }
    }

    Entry::Entry(const pql::ast::Declaration* declaration, const std::string& val, EntryType type)
    {
        this->m_declaration = declaration;
        this->m_val = val;
        this->m_type = type;
    }

    Entry::Entry(const pql::ast::Declaration* declaration, const simple::ast::StatementNum& val)
    {
        if(declaration->design_ent == ast::DESIGN_ENT::VARIABLE ||
            declaration->design_ent == ast::DESIGN_ENT::PROCEDURE ||
            declaration->design_ent == ast::DESIGN_ENT::CONSTANT)
        {
            throw util::PqlException("pql::eval::table::Entry",
                "Entry for {} should be instantiated using string instead of stmt num", declaration->toString());
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
    const ast::Declaration* Entry::getDeclaration() const
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


    int Join::get_next_id()
    {
        static int next_id = 0;
        next_id++;
        return next_id;
    };

    int Join::getId() const
    {
        return this->m_id;
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
        this->m_id = Join::get_next_id();
        util::logfmt("pql::eval::table::join", "Creating join with id {}", this->m_id);
    }
    bool Join::isAllowedEntry(const std::pair<Entry, Entry>& entry) const
    {
        return m_allowed_entries.count(entry) > 0;
    }

    pql::ast::Declaration* Join::getDeclA() const
    {
        return this->m_decl_a;
    }

    pql::ast::Declaration* Join::getDeclB() const
    {
        return this->m_decl_b;
    }

    const std::unordered_set<std::pair<Entry, Entry>>& Join::getAllowedEntries() const
    {
        return this->m_allowed_entries;
    }

    std::unordered_set<std::pair<Entry, Entry>>& Join::getAllowedEntries()
    {
        return this->m_allowed_entries;
    }

    void Join::setAllowedEntries(const std::unordered_set<std::pair<Entry, Entry>>& allowed_entries)
    {
        this->m_allowed_entries = allowed_entries;
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


    void Table::putDomain(const ast::Declaration* decl, Domain entries)
    {
        util::logfmt("pql::eval::table", "Updating domain of {} with {} entries", decl->toString(), entries.size());
        m_domains[decl] = std::move(entries);
    }
    void Table::addJoin(const Join& join)
    {
        m_joins.push_back(join);
    }

    void Table::addSelectDecl(const ast::Declaration* decl)
    {
        spa_assert(decl);
        m_select_decls.insert(decl);
    }

    std::unordered_set<Entry> Table::getDomain(const ast::Declaration* decl) const
    {
        auto it = m_domains.find(decl);
        if(it == m_domains.end())
            return std::unordered_set<Entry>();
        return it->second;
    }

    bool Table::hasValidDomain() const
    {
        for(const ast::Declaration* decl : m_select_decls)
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


    Entry Table::extractAttr(const Entry& entry, const ast::AttrRef& attr_ref, const pkb::ProgramKB* pkb)
    {
        const ast::Declaration* decl = attr_ref.decl;
        Entry extracted_entry = entry;

        if(attr_ref.attr_name == ast::AttrName::kProcName)
        {
            if(decl->design_ent == ast::DESIGN_ENT::CALL)
            {
                const pkb::Statement& stmt = pkb->getStatementAt(entry.getStmtNum());
                const simple::ast::Stmt* ast_smt = stmt.getAstStmt();
                const simple::ast::ProcCall* call_ast_stmt = dynamic_cast<const simple::ast::ProcCall*>(ast_smt);
                // The corresponding stmt should always be call stmt
                spa_assert(call_ast_stmt);
                std::string proc_name = call_ast_stmt->proc_name;
                if(proc_name.empty())
                    throw util::PqlException("pql::eval::table",
                        "Stmt at {} has empty call proc name. Make sure that the stmt is call stmt and the callee "
                        "proc_name has been properly populated",
                        entry.getStmtNum());
                extracted_entry = Entry(decl, proc_name, EntryType::kProc);
            }
            else if(decl->design_ent == ast::DESIGN_ENT::PROCEDURE)
            {
                // The proc declaration should have procName entries
                spa_assert(entry.getType() == EntryType::kProc);
            }
            else
            {
                // No other design entities are allowed to have 'procName' AttrName. This should be enforced on the
                // parser layer
                unreachable();
            }
        }
        else if(attr_ref.attr_name == ast::AttrName::kVarName)
        {
            if(decl->design_ent == ast::DESIGN_ENT::READ || decl->design_ent == ast::DESIGN_ENT::PRINT)
            {
                const pkb::Statement& stmt = pkb->getStatementAt(entry.getStmtNum());
                const simple::ast::Stmt* ast_stmt = stmt.getAstStmt();
                const simple::ast::ReadStmt* ast_read_stmt = dynamic_cast<const simple::ast::ReadStmt*>(ast_stmt);
                const simple::ast::PrintStmt* ast_print_stmt = dynamic_cast<const simple::ast::PrintStmt*>(ast_stmt);
                if(ast_read_stmt)
                {
                    extracted_entry = Entry(decl, ast_read_stmt->var_name, EntryType::kVar);
                }
                else if(ast_print_stmt)
                {
                    extracted_entry = Entry(decl, ast_print_stmt->var_name, EntryType::kVar);
                }
                else
                {
                    // the corresponding ast Stmt should always be ReadStmt/PrintStmt
                    unreachable();
                }
            }
            else if(decl->design_ent == ast::DESIGN_ENT::VARIABLE)
            {
                // For variable declaration the entry should already contain the varName
                spa_assert(extracted_entry.getType() == EntryType::kVar);
            }
            else
            {
                // No other design entities are allowed to have 'varName' AttrName. This should be enforced on the
                // parser layer
                unreachable();
            }
        }
        else if(attr_ref.attr_name == ast::AttrName::kValue)
        {
            // Only const declaration is allowed to have 'value' AttrName (enforced on parser) and the entry should
            // already be populated with the value
            spa_assert(decl->design_ent == ast::DESIGN_ENT::CONSTANT && extracted_entry.getType() == EntryType::kConst);
        }
        else if(attr_ref.attr_name == ast::AttrName::kStmtNum)
        {
            // Only statement-like entites declarations are allowed to have 'stmt#' AttrName (enforced on parser) and
            // the entry should already be populated with the stmt
            spa_assert(
                ast::getStmtDesignEntities().count(decl->design_ent) && extracted_entry.getType() == EntryType::kStmt);
        }
        else
        {
            // ast::AttrName::kInvalid should never be present
            unreachable();
        }

        util::logfmt("pql::parser::table", "{} causes  {} to be extracted to {}.", attr_ref.toString(),
            entry.toString(), extracted_entry.toString());

        return extracted_entry;
    }

    static std::string format_row_to_output(
        const solver::IntRow& row, const std::vector<ast::Elem>& return_tuple, const pkb::ProgramKB* pkb)
    {
        util::logfmt("pql::parser::table", "Extracting result from row: {}", row.toString());

        size_t ctr = 0;
        std::string ret {};
        // ret.reserve(return_tuple.size() * 3);

        for(const ast::Elem& elem : return_tuple)
        {
            spa_assert(elem.isAttrRef() || elem.isDeclaration());
            const ast::Declaration* decl = elem.isDeclaration() ? elem.declaration() : elem.attrRef().decl;
            const auto& entry = row.getVal(decl);

            if(elem.isDeclaration())
            {
                if(entry.getType() == EntryType::kStmt)
                    ret += std::to_string(entry.getStmtNum());
                else
                    ret += entry.getVal();
            }
            else
            {
                auto attr = Table::extractAttr(entry, elem.attrRef(), pkb);
                if(attr.getType() == EntryType::kStmt)
                    ret += std::to_string(attr.getStmtNum());
                else
                    ret += attr.getVal();
            }

            if(ctr + 1 < return_tuple.size())
                ret += " ";

            ctr++;
        }

        return ret;
    }

    static bool check_conflicting_values(const Table::ValueAssignmentMap& values, const Entry& ent)
    {
        if(auto it = values.find(ent.getDeclaration()); it != values.end())
            return it->second != ent;
        return false;
    }

    bool Table::evaluateJoinValues(ValueAssignmentMap& values, const ast::Declaration* this_decl, size_t join_idx,
        const std::vector<const Join*>& joins, JoinIdSet& visited_joins, DeclJoinMap& join_map)
    {
        if(join_idx >= joins.size())
            return true;

        auto join = joins[join_idx];
        if(visited_joins.count(join->getId()) > 0)
            return this->evaluateJoinValues(values, this_decl, join_idx + 1, joins, visited_joins, join_map);

        auto& allowed = join->getAllowedEntries();
        if(allowed.empty())
            return false;

        auto decl_a = join->getDeclA();
        auto decl_b = join->getDeclB();

        visited_joins.insert(join->getId());

        for(auto& [a, b] : allowed)
        {
            if(check_conflicting_values(values, a) || check_conflicting_values(values, b))
                continue;

            if(m_domains[decl_a].count(a) == 0 || m_domains[decl_b].count(b) == 0)
                continue;

            auto other_decl = (this_decl == decl_a ? decl_b : decl_a);

            auto values_copy = values;
            values_copy[decl_a] = a;
            values_copy[decl_b] = b;

            /*
                we need to make a copy here, because both `recursivelyTraverseJoins` and `evaluateJoinValues`
                can potentially add decl values to the value assignment. if they fail at some deeper depth,
                we have no way of knowing which decl assignments to remove, causing backtracking failure.

                by making a copy (and just assigning the copy if we succeed, or yeeting it if we fail), we
                avoid this problem entirely.
            */
            auto visited_copy = visited_joins;

            /*
                These two functions are actually mutually recursive. This is because if a declaration has more
                than one join that involves it, then we need a way to backtrack and retry the first join
                if the second join fails.

                If we think of it as a tree DFS (which it is), then we need to ensure that we can retry all
                prior branches (with a different assignment) if a given branch fails; the recursive call to
                `evaluateJoinValues` ensures that this can happen; a naive loop over joins would not let us
                retry assignments from previous joins.
            */
            if(this->recursivelyTraverseJoins(values_copy, visited_copy, other_decl, join_map))
            {
                if(this->evaluateJoinValues(values_copy, this_decl, join_idx + 1, joins, visited_copy, join_map))
                {
                    values = std::move(values_copy);
                    return true;
                }
            }
        }

        return false;
    }

    bool Table::recursivelyTraverseJoins(
        ValueAssignmentMap& values, JoinIdSet& visited_joins, const ast::Declaration* this_decl, DeclJoinMap& join_map)
    {
        spa_assert(join_map.count(this_decl) > 0);

        if(m_domains[this_decl].empty())
            return false;

        return this->evaluateJoinValues(values, this_decl, 0, join_map[this_decl], visited_joins, join_map);
    }

    bool Table::validateAssignments(ValueAssignmentMap& values, const std::vector<Join>& joins)
    {
        for(auto& join : joins)
        {
            auto decl_a = join.getDeclA();
            auto decl_b = join.getDeclB();

            // the joins might be involving other decls not in the current connected component, so
            // ignore them for now.
            if(values.count(decl_a) == 0 || values.count(decl_b) == 0)
                continue;

            if(join.getAllowedEntries().count({ values.at(decl_a), values.at(decl_b) }) == 0)
                return false;
        }

        return true;
    }

    bool Table::evaluateJoinsOverDomains()
    {
        if(m_select_decls.empty())
            return true;

        // make a vector of them, so we (a) can index, and (b) have a consistent order.
        std::unordered_map<const ast::Declaration*, std::vector<const Join*>> join_mapping {};
        for(auto& join : m_joins)
        {
            join_mapping[join.getDeclA()].push_back(&join);
            join_mapping[join.getDeclB()].push_back(&join);
        }

        for(auto& [decl, joins] : join_mapping)
        {
            std::sort(joins.begin(), joins.end(), [](const auto* a, const auto* b) -> bool {
                return a->getAllowedEntries().size() < b->getAllowedEntries().size();
            });
        }

        auto graph = solver::DepGraph(m_select_decls, m_joins);
        auto components = graph.getComponents();

        for(auto& comp : components)
        {
            ValueAssignmentMap assignments {};
            spa_assert(comp.size() > 0);

            auto first_decl = *comp.begin();

            if(join_mapping.count(first_decl) == 0)
            {
                // if a decl has no joins, by definition it must be its own connected component.
                spa_assert(comp.size() == 1);

                // in this case, any value will do, so we continue to the next component as long
                // as its domain is nonzero.
                if(m_domains[first_decl].empty())
                    return false;

                continue;
            }

            std::unordered_set<int> visited_joins {};
            if(!this->recursivelyTraverseJoins(assignments, visited_joins, first_decl, join_mapping))
                return false;

            // for sanity, verify that the assignments we produced satisfy the join conditions.
            // we only need to validate this connected component of decls, since other decls can't
            // affect the correctness of this assignment (by definition).
            if(!this->validateAssignments(assignments, m_joins))
                return false;
        }

        return true;
    }







    std::list<std::string> Table::getFailedResult(const ast::ResultCl& result)
    {
        static const std::list<std::string> FalseResult = std::list<std::string> { "FALSE" };
        return result.isBool() ? FalseResult : std::list<std::string> {};
    }

    std::list<std::string> Table::getResult(const ast::ResultCl& result_cl, const pkb::ProgramKB* pkb)
    {
        START_BENCHMARK_TIMER("Table get result");
        util::logfmt("pql::eval::table", "Starting to get {} for table {}.", result_cl.toString(), toString());


        std::unordered_set<const ast::Declaration*> ret_cols;
        if(result_cl.isTuple())
        {
            for(const ast::Elem& elem : result_cl.tuple())
            {
                util::logfmt("pql::eval::table", "Adding {} to columns", elem.toString());
                if(elem.isDeclaration())
                {
                    ret_cols.insert(elem.declaration());
                }
                else if(elem.isAttrRef())
                {
                    ret_cols.insert(elem.attrRef().decl);
                }
            }
        }
        else
        {
            static constexpr bool DFS_JOIN_IMPLEMENTATION = true;

            if constexpr(DFS_JOIN_IMPLEMENTATION)
            {
                // check that all select domains are nonzero first so we can
                // skip traversing any joins for the trivial case.
                for(auto decl : m_select_decls)
                    if(m_domains[decl].empty())
                        return { "FALSE" };

                if(this->evaluateJoinsOverDomains())
                    return { "TRUE" };

                else
                    return { "FALSE" };
            }
        }

        // we don't need the domains after this, so move it out.
        solver::Solver solver(
            /* joins: */ m_joins, /* domains: */ std::move(m_domains), /* return_decls: */ ret_cols,
            /* select_decls: */ m_select_decls);

        if(solver.isValid())
        {
            if(result_cl.isBool())
                return std::list<std::string> { "TRUE" };
        }
        else
        {
            return Table::getFailedResult(result_cl);
        }

        solver::IntTable ret_tbl = solver.getRetTbl();
        if(ret_tbl.empty())
        {
            return Table::getFailedResult(result_cl);
        }

        std::list<std::string> result {};
        {
            START_BENCHMARK_TIMER("converting rows to strings");
            auto result_tup = result_cl.tuple();

            for(auto& row : ret_tbl.getRows())
                result.push_back(format_row_to_output(row, result_tup, pkb));
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
