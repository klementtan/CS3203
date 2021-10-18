#include "pql/eval/solver.h"
#include "exceptions.h"
#include "zpr.h"
#include <queue>
#include <cassert>
#include <utility>
#include "util.h"
#include "timer.h"

namespace pql::eval::solver
{
    // Intermediate Row for IntTable
    IntRow::IntRow(std::unordered_map<const ast::Declaration*, table::Entry> columns)
        : m_columns(std::move(columns)) { }
    IntRow::IntRow() : m_columns() { }
    // merge this row with a new column and return a new copy
    IntRow IntRow::addColumn(const ast::Declaration* decl, const table::Entry& entry) const
    {
        if(m_columns.count(decl))
            throw util::PqlException(
                "pql::eval::solver", "decl:{} already exist in IntRow:{}", decl->toString(), toString());
        // use copy assignment to make a new column
        std::unordered_map<const ast::Declaration*, table::Entry> new_columns = m_columns;
        new_columns[decl] = entry;
        return IntRow(new_columns);
    }

    bool IntRow::contains(const ast::Declaration* decl) const
    {
        return m_columns.count(decl);
    }
    table::Entry IntRow::getVal(const ast::Declaration* decl) const
    {
        if(!contains(decl))
        {
            throw util::PqlException(
                "pql::eval::table", "IntRow:{} does not contain decl:{}", toString(), decl->toString());
        }
        return m_columns.find(decl)->second;
    }
    // check columns in the row exist in one of the allowed joins
    bool IntRow::isAllowed(const table::Join& join) const
    {
        const ast::Declaration* decl_a = join.getDeclA();
        const ast::Declaration* decl_b = join.getDeclB();
        if(!(contains(decl_a) && contains(decl_b)))
        {
            throw util::PqlException("pql::eval::solver",
                "Fail to check if {} is valid against {}. {} or {} is not in IntRow", toString(), join.toString(),
                decl_a->toString(), decl_b->toString());
        }
        const std::pair<table::Entry, table::Entry> curr_entry =
            std::make_pair(m_columns.find(decl_a)->second, m_columns.find(decl_b)->second);
        // curr_entries just needs to exist in one of the set of allowed entries
        return join.isAllowedEntry(curr_entry);
    }
    std::unordered_set<const ast::Declaration*> IntRow::getHeaders() const
    {
        std::unordered_set<const ast::Declaration*> headers;
        for(const auto& [decl, _] : m_columns)
        {
            headers.insert(decl);
        }
        return headers;
    }

    bool IntRow::canMerge(const IntRow& other) const
    {
        std::unordered_set<const ast::Declaration*> other_headers = other.getHeaders();
        for(const auto& decl : other_headers)
        {
            if(m_columns.count(decl))
            {
                const table::Entry& entry = m_columns.find(decl)->second;
                const table::Entry other_entry = other.getVal(decl);
                // cannot merge as there are conflicting entries for the same decl
                if(entry != other_entry)
                    return false;
            }
        }
        return true;
    }

    IntRow IntRow::mergeRow(const IntRow& other) const
    {
        assert(canMerge(other));
        std::unordered_set<const ast::Declaration*> other_headers = other.getHeaders();
        std::unordered_map<const ast::Declaration*, table::Entry> new_columns = m_columns;
        for(const auto& decl : other_headers)
        {
            if(m_columns.count(decl) == 0)
            {
                new_columns[decl] = other.getVal(decl);
            }
        }
        return IntRow(new_columns);
    }

    std::string IntRow::toString() const
    {
        std::string ret = "IntRow(\n";
        for(const auto& [decl, entry] : m_columns)
        {
            ret += zpr::sprint("({}: {})\n", decl->toString(), entry.toString());
        }
        ret += ")";
        return ret;
    }


    IntTable::IntTable(std::vector<IntRow> rows, const std::unordered_set<const ast::Declaration*>& headers)
        : m_rows(std::move(rows)), m_headers(headers)
    {
        // TOOD: Remove this sanity check during release
        for(const auto& row : m_rows)
        {
            if(row.getHeaders().size() != headers.size())
                throw util::PqlException("pql::eval::solver",
                    "Initialised {} with mismatched rows({}) and headers. Row header size:{} vs provided header "
                    "size:{}",
                    toString(), row.toString(), row.getHeaders().size(), headers.size());

            for(const ast::Declaration* header : row.getHeaders())
            {
                if(headers.count(header) == 0)
                    throw util::PqlException(
                        "pql::eval::solver", "Initialised {} with mismatched rows and headers", toString());
            }
        }
    }

    IntTable::IntTable()
        : // Initialise empty table with a row with no columns
          m_rows({ IntRow() }), m_headers()
    {
    }

    bool IntTable::contains(const ast::Declaration* declaration)
    {
        return m_headers.count(declaration);
    }

    IntTable IntTable::merge(const IntTable& other)
    {
        // use copy assignment to create new rows
        std::vector<IntRow> new_rows;
        // m_rows should never be empty. Empty IntTable should contain an empty IntRow with no columns
        if(m_rows.empty())
        {
            util::logfmt("pql::eval::solver", "Detected IntTbl in {}. IntTbl will always be invalid", toString());
        }
        for(const auto& this_row : m_rows)
        {
            for(const auto& other_row : other.getRows())
            {
                if(this_row.canMerge(other_row))
                {
                    new_rows.emplace_back(this_row.mergeRow(other_row));
                }
            }
        }
        std::unordered_set<const ast::Declaration*> new_headers = m_headers;
        for(const ast::Declaration* header : other.getHeaders())
        {
            new_headers.insert(header);
        }
        return IntTable(new_rows, new_headers);
    }

    IntTable IntTable::mergeColumn(const ast::Declaration* decl, const table::Domain& domain) const
    {
        if(m_headers.count(decl))
        {
            throw util::PqlException("pql::eval::solver",
                "Failed to mergeColumn decl:{} with {}. Decl already in headers", decl->toString(), toString());
        }
        // use copy assignment to create new rows
        std::vector<IntRow> new_rows;
        if(m_rows.empty())
        {
            util::logfmt("pql::eval::solver", "Merging {} to emtpy tbl {} will always result in empty table",
                decl->toString(), toString());
        }
        for(const auto& row : m_rows)
        {
            for(const auto& entry : domain)
            {
                if(entry.getDeclaration() != decl)
                    throw util::PqlException("pql::eval::solver",
                        "Failed to merge column to {} due to conflicting decl:{} and provided entry: {}", toString(),
                        decl->toString(), entry.getDeclaration()->toString());
                IntRow new_row = row.addColumn(decl, entry);
                util::logfmt("pql::eval::solver", "New row:{} from merging {} with {}", new_row.toString(),
                    row.toString(), entry.toString());
                new_rows.emplace_back(row.addColumn(decl, entry));
            }
        }
        std::unordered_set<const ast::Declaration*> new_headers = m_headers;
        new_headers.insert(decl);
        return IntTable(new_rows, new_headers);
    }
    std::unordered_set<const ast::Declaration*> IntTable::getHeaders() const
    {
        return this->m_headers;
    }
    std::vector<IntRow> IntTable::getRows() const
    {
        return this->m_rows;
    }

    void IntTable::filterRows(const table::Join& join)
    {
        const ast::Declaration* decl_a = join.getDeclA();
        const ast::Declaration* decl_b = join.getDeclB();
        if(!(m_headers.count(decl_a) && m_headers.count(decl_b)))
        {
            util::logfmt("pql::eval::solver", "Skipping filtering {} on {} as it does not contain {} or {}",
                join.toString(), toString(), decl_a->toString(), decl_b->toString());
            return;
        }
        auto it = m_rows.begin();
        while(it != m_rows.end())
        {
            if(it->isAllowed(join))
            {
                it++;
            }
            else
            {
                util::logfmt("pql::eval::solver", "Filtering {} from {}. Removing IntRow {}.", join.toString(),
                    toString(), it->toString());
                it = m_rows.erase(it);
            }
        }
    }
    bool IntTable::empty() const
    {
        return m_rows.empty() ||
               // contains an empty row
               (m_rows.size() == 1 && m_rows.front().getHeaders().empty());
    }

    std::string IntTable::toString() const
    {
        std::string ret = "IntTable(\n";
        ret += "m_headers: [\n";
        for(const auto& header : m_headers)
        {
            ret += zpr::sprint("{}, ", header->toString());
        }
        ret += "]\nm_rows:[";
        for(const auto& row : m_rows)
        {
            ret += zpr::sprint("{}, ", row.toString());
        }
        ret += "])";
        return ret;
    }

    DepGraph::DepGraph(const std::unordered_set<const ast::Declaration*>& decls, std::vector<table::Join> joins)
        : m_joins(std::move(joins)), m_colouring(), m_graph()
    {
        util::logfmt("pql::eval::solver", "Constructing Dependency Graph");
        for(const auto& join : m_joins)
        {
            util::logfmt("pql::eval::solver::dep_g", "Adding {} to {}", join.getDeclA()->toString(),
                join.getDeclB()->toString());
            util::logfmt("pql::eval::solver::dep_g", "Adding {} to {}", join.getDeclB()->toString(),
                join.getDeclA()->toString());
            m_graph[join.getDeclA()].insert(join.getDeclB());
            m_graph[join.getDeclB()].insert(join.getDeclA());
        }
        int colour = 0;
        for(const auto& decl : decls)
        {
            if(m_colouring.count(decl))
                continue;
            colour_node(decl, colour);
            colour++;
        }
        util::logfmt("pql::eval::solver", "Complete constructing dependency graph: {}", toString());
    }

    void DepGraph::colour_node(const ast::Declaration* s, int colour)
    {
        std::queue<const ast::Declaration*> q;
        q.push(s);
        while(!q.empty())
        {
            const ast::Declaration* u = q.front();
            util::logfmt("pql::eval::solver", "Colouring {} {}", u->toString(), colour);
            q.pop();
            if(m_colouring.count(u) == 1)
                continue;
            m_colouring[u] = colour;
            for(const auto& v : m_graph[u])
            {
                if(m_colouring.count(v) == 1)
                    continue;
                q.push(v);
            }
        }
    }
    std::string DepGraph::toString() const
    {
        std::string ret = "DepGraph(";
        for(const auto& [decl, colour] : m_colouring)
        {
            ret += zpr::sprint("{}: {}", colour, decl->toString());
        }
        return ret;
    }

    std::vector<std::unordered_set<const ast::Declaration*>> DepGraph::getComponents() const
    {
        std::unordered_map<int, std::unordered_set<const ast::Declaration*>> colours;
        for(const auto& [decl, colour] : m_colouring)
        {
            colours[colour].insert(decl);
        }
        std::vector<std::unordered_set<const ast::Declaration*>> ret;
        for(const auto& [_, comp] : colours)
        {
            ret.emplace_back(comp);
        }
        return ret;
    }

    static std::unordered_set<const ast::Declaration*> mergeAndCopySet(
        std::unordered_set<const ast::Declaration*> a, const std::unordered_set<const ast::Declaration*>& b)
    {
        std::unordered_set<const ast::Declaration*> ret(std::move(a));
        for(const ast::Declaration* decl : b)
        {
            ret.insert(decl);
        }
        return ret;
    }
    Solver::Solver(const std::vector<table::Join>& joins,
        std::unordered_map<const ast::Declaration*, table::Domain> domains,
        const std::unordered_set<const ast::Declaration*>& return_decls,
        const std::unordered_set<const ast::Declaration*>& select_decls)
        : m_domains(std::move(domains)), m_joins(joins), m_return_decls(return_decls), m_int_tables(),
          m_decl_components(), m_dep_graph(mergeAndCopySet(return_decls, select_decls), joins)
    {
        START_BENCHMARK_TIMER("Solver constructor");
        m_decl_components = m_dep_graph.getComponents();

        trim(return_decls);
        trim(select_decls);

        // all declaration should start as table initially
        for(const ast::Declaration* decl : return_decls)
        {
            START_BENCHMARK_TIMER("Create initial table for return declarations");
            IntTable tbl;
            if(m_domains.count(decl) == 0)
                throw util::PqlException("pql::eval::solver", "{} does not have any domain", decl->toString());
            tbl = tbl.mergeColumn(decl, m_domains.find(decl)->second);
            m_int_tables.push_back(tbl);
            util::logfmt("pql::eval::solver", "Adding {} to m_int_tables", tbl.toString());
        }
        // all declaration should start as table initially
        for(const ast::Declaration* decl : select_decls)
        {
            START_BENCHMARK_TIMER("Create initial table for select declarations");
            IntTable tbl;
            if(m_domains.count(decl) == 0)
                throw util::PqlException("pql::eval::solver", "{} does not have any domain", decl->toString());
            if(has_table(decl))
            {
                // IntTbl already initialized as decl is also a ret decl.
                continue;
            }
            tbl = tbl.mergeColumn(decl, m_domains.find(decl)->second);
            m_int_tables.push_back(tbl);
            util::logfmt("pql::eval::solver", "Adding {} to m_int_tables", tbl.toString());
        }
        preprocess_int_table();
    }
    std::vector<table::Join> Solver::get_joins(const ast::Declaration* decl) const
    {
        std::vector<table::Join> ret;
        for(const table::Join& join : m_joins)
        {
            if(join.getDeclA() == decl || join.getDeclB() == decl)
                ret.push_back(join);
        }
        return ret;
    }

    void Solver::trim_helper(const ast::Declaration* decl, table::Join& join)
    {
        util::logfmt("pql::eval::solver", "Trimming {} and {}", decl->toString(), join.toString());
        if(m_domains.count(decl) == 0)
            throw util::PqlException("pql::solver::eval",
                "Failed to trim {} with {}. Declaration's domain not initialized", decl->toString(), join.toString());
        const std::unordered_set<table::Entry>& domain = m_domains.find(decl)->second;
        std::unordered_set<std::pair<table::Entry, table::Entry>> join_allowed_entries = join.getAllowedEntries();
        // Extract out decls' Entry from join
        std::unordered_set<table::Entry> decl_join_allowed_entries;
        for(const auto& [entry_a, entry_b] : join_allowed_entries)
        {
            if(entry_a.getDeclaration() == decl)
            {
                decl_join_allowed_entries.insert(entry_a);
            }
            else if(entry_b.getDeclaration() == decl)
            {
                decl_join_allowed_entries.insert(entry_b);
            }
            else
            {
                throw util::PqlException("pql::eval::solver",
                    "Failed to trim {} with {}. Join does not involve the declaration.", decl->toString(),
                    join.toString());
            }
        }
        // find the entries that are in both domain and joins
        std::unordered_set<table::Entry> entry_set_intersect;
        for(const auto& entry : domain)
        {
            if(decl_join_allowed_entries.count(entry))
                entry_set_intersect.insert(entry);
        }
        for(const auto& entry : decl_join_allowed_entries)
        {
            if(domain.count(entry))
                entry_set_intersect.insert(entry);
        }
        // Update domain with the new trimmed domain
        m_domains[decl] = entry_set_intersect;
        // remove allowed entries to only contain trimmed entries
        auto join_it = join_allowed_entries.begin();
        while(join_it != join_allowed_entries.end())
        {
            const table::Entry& entry = join_it->first.getDeclaration() == decl ? join_it->first : join_it->second;
            // remove allowed entries that are not in intersect
            if(entry_set_intersect.count(entry) == 0)
            {
                util::logfmt("pql::eval::solver", "Removing {} from {}.", entry.toString(), join.toString());
                join_it = join_allowed_entries.erase(join_it);
            }
            else
            {
                join_it++;
            }
        }
        join.setAllowedEntries(join_allowed_entries);
    }

    void Solver::trim(const std::unordered_set<const ast::Declaration*>& decls)
    {
        START_BENCHMARK_TIMER("Trimming declarations and joins");
        util::logfmt("pql::eval::solver", "Trimming declarations");
        for(table::Join& join : m_joins)
        {
            if(decls.count(join.getDeclA()))
                trim_helper(join.getDeclA(), join);
            if(decls.count(join.getDeclB()))
                trim_helper(join.getDeclB(), join);
        }
    }

    bool Solver::has_table(const ast::Declaration* decl) const
    {
        for(const auto& tbl : m_int_tables)
        {
            if(tbl.getHeaders().count(decl))
                return true;
        }
        return false;
    }

    size_t Solver::get_table_index(const ast::Declaration* decl) const
    {
        std::vector<size_t> ret;
        for(size_t i = 0; i < m_int_tables.size(); i++)
        {
            if(m_int_tables[i].getHeaders().count(decl))
                ret.emplace_back(i);
        }
        if(ret.size() > 1)
            throw util::PqlException(
                "pql::eval::solver", "{} belongs to more than one IntTable. IntTbl: {}", decl->toString(), toString());
        if(ret.empty())
            throw util::PqlException("pql::eval::solver", "{} belongs none of the IntTable", decl->toString());
        return ret.front();
    }

    // update m_int_tables with tables that corresponds to a comp
    void Solver::preprocess_int_table()
    {
        START_BENCHMARK_TIMER("Preprocess initial table");
        util::logfmt("pql::eval::solver", "Starting pre-process");
        std::unordered_set<const ast::Declaration*> processed_decl;
        std::vector<IntTable> new_int_tables;
        std::unordered_set<int> processed_join;

        for(const std::unordered_set<const ast::Declaration*>& component : m_decl_components)
        {
            IntTable new_table;
            // A component should never be empty
            assert(!component.empty());
            std::string log = "{";
            for(const ast::Declaration* decl : component)
                log += zpr::sprint("{}, ", decl->toString());
            log += "}";
            util::logfmt("pql::eval::solver", "Merging component {}", log);
            // TODO: we should sort in increasing order of table size
            for(const ast::Declaration* decl : component)
            {
                IntTable& prev_table = m_int_tables[get_table_index(decl)];
                // merge to new table if it has not been processed
                if(new_table.getHeaders().count(decl) == 0)
                {
                    util::logfmt("pql::eval::solver", "merging decl {} from {} into {}", decl->toString(),
                        prev_table.toString(), new_table.toString());
                    new_table = new_table.merge(prev_table);
                }

                START_BENCHMARK_TIMER(zpr::sprint("**** filtered joins for {}", decl->name));
                std::vector<table::Join> joins = get_joins(decl);
                for(const table::Join& join : joins)
                {
                    if(processed_join.count(join.getId()))
                    {
                        util::logfmt("pql::eval::solver",
                            "Skipping filter join with id {} as it has already been processed.", join.getId());
                        continue;
                    }

                    const ast::Declaration* other_decl = join.getDeclA() == decl ? join.getDeclB() : join.getDeclA();
                    if(new_table.getHeaders().count(other_decl) == 0)
                    {
                        IntTable& other_prev_table = m_int_tables[get_table_index(other_decl)];
                        util::logfmt(
                            "pql::eval::solver", "Merging {} to {}", other_prev_table.toString(), new_table.toString());
                        new_table = new_table.merge(other_prev_table);
                    }
                    new_table.filterRows(join);
                    processed_join.insert(join.getId());
                }
            }
            util::logfmt("pql::eval::solver", "New final merged table for component {}", new_table.toString());
            new_int_tables.push_back(new_table);
        }
        m_int_tables = new_int_tables;
        util::logfmt("pql::eval::solver", "Solver after preprocessing {}", toString());
    }

    bool Solver::isValid() const
    {
        for(const IntTable& tbl : m_int_tables)
        {
            if(tbl.empty())
            {
                return false;
            }
        }
        return true;
    }

    std::string Solver::toString() const
    {
        std::string ret = "Solver(";
        for(const IntTable& tbl : m_int_tables)
        {
            ret += zpr::sprint("\t{}\n", tbl.toString());
        }
        ret += ")\n";
        return ret;
    }
    IntTable Solver::getRetTbl()
    {
        START_BENCHMARK_TIMER("Create return table");
        util::logfmt("pql::eval::solver", "Getting return table");
        IntTable ret_table;
        assert(!m_return_decls.empty());
        for(const ast::Declaration* decl : m_return_decls)
        {
            util::logfmt("pql::eval::solver", "Handling return decl {}", decl->toString());
            // already added into table by another decl in the same component
            if(ret_table.getHeaders().count(decl))
                continue;
            IntTable& prev_table = m_int_tables[get_table_index(decl)];
            util::logfmt("pql::eval::solver", "Merging table {} to {}", ret_table.toString(), prev_table.toString());
            ret_table = ret_table.merge(prev_table);
        }
        util::logfmt("pql::eval::solver", "Return table: {}", ret_table.toString());
        return ret_table;
    }
}
