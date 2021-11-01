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
                throw util::PqlException("pql::eval::table::Entry",
                    "Entry for {} should be instantiated using stmt num instead of string", declaration->toString());
        }
    }

    Entry::Entry(pql::ast::Declaration* declaration, const std::string& val, EntryType type)
    {
        this->m_declaration = declaration;
        this->m_val = val;
        this->m_type = type;
    }

    Entry::Entry(pql::ast::Declaration* declaration, const simple::ast::StatementNum& val)
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


    void Table::putDomain(ast::Declaration* decl, const std::unordered_set<Entry>& entries)
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
        spa_assert(decl);
        m_select_decls.insert(decl);
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

    std::string tupleToString(const std::vector<Entry>& tuple)
    {
        std::string ret { "Tuple:[\n" };
        for(auto entry : tuple)
        {
            ret += zpr::sprint("{},", entry.toString());
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


    Entry Table::extractAttr(const Entry& entry, const ast::AttrRef& attr_ref, const pkb::ProgramKB* pkb)
    {
        ast::Declaration* decl = attr_ref.decl;
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

    std::vector<Entry> extract_result(
        const solver::IntRow& row, const std::vector<ast::Elem>& return_tuple, const pkb::ProgramKB* pkb)
    {
        util::logfmt("pql::parser::table", "Extracting result from row: {}", row.toString());
        std::vector<Entry> tuple;
        for(const ast::Elem& elem : return_tuple)
        {
            spa_assert(elem.isAttrRef() || elem.isDeclaration());
            ast::Declaration* decl = elem.isDeclaration() ? elem.declaration() : elem.attrRef().decl;
            Entry entry = row.getVal(decl);

            if(elem.isDeclaration())
            {
                tuple.push_back(entry);
            }
            else
            {
                tuple.push_back(Table::extractAttr(entry, elem.attrRef(), pkb));
            }
        }
        util::logfmt("pql::pareser::tabler", "Extracted tuple from row {}", tupleToString(tuple));
        return tuple;
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


        std::vector<ast::Declaration*> ret_cols;
        if(result_cl.isTuple())
        {
            for(const ast::Elem& elem : result_cl.tuple())
            {
                util::logfmt("pql::eval::table", "Adding {} to columns", elem.toString());
                if(elem.isDeclaration())
                {
                    ret_cols.push_back(elem.declaration());
                }
                else if(elem.isAttrRef())
                {
                    ret_cols.push_back(elem.attrRef().decl);
                }
            }
        }

        std::unordered_set<const ast::Declaration*> select_decls;
        for(const ast::Declaration* decl : m_select_decls)
        {
            select_decls.insert(decl);
        }
        std::unordered_set<const ast::Declaration*> ret_decls;
        for(const ast::Declaration* decl : ret_cols)
        {
            ret_decls.insert(decl);
        }

        std::unordered_map<const ast::Declaration*, table::Domain> domains;
        for(const auto& [decl, domain] : m_domains)
        {
            domains[decl] = domain;
        }
        solver::Solver solver(
            /** joins=*/m_joins, /**domains=*/domains, /**return_decls=*/ret_decls, /**select_decls=*/select_decls);

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

        std::vector<std::vector<Entry>> result_entries(ret_tbl.size());

        {
            START_BENCHMARK_TIMER("Populating return table into result entries");
            for(int i = 0; i < ret_tbl.size(); i++)
            {
                result_entries[i] = extract_result(ret_tbl.getRow(i), result_cl.tuple(), pkb);
            }
        }

        std::list<std::string> result;

        {
            START_BENCHMARK_TIMER("Populating result entries into result (vector<string>)");
            for(const std::vector<Entry>& entries : result_entries)
            {
                std::vector<std::string> curr_results;
                for(const Entry& entry : entries)
                {
                    curr_results.push_back(
                        entry.getType() == EntryType::kStmt ? std::to_string(entry.getStmtNum()) : entry.getVal());
                }
                std::string merged_result = std::accumulate(curr_results.begin(), curr_results.end(), std::string {},
                    [](const std::string& a, const std::string& b) { return a.empty() ? b : a + " " + b; });
                util::logfmt("pql::eval::table", "Adding \"{}\" to result", merged_result);
                result.push_back(merged_result);
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
