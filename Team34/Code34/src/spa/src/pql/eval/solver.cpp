// solver.cpp

#include <queue>
#include <utility>
#include <iterator>
#include <algorithm>

#include "zpr.h"
#include "util.h"
#include "timer.h"
#include "exceptions.h"
#include "pql/eval/solver.h"

namespace pql::eval::solver
{
    // Intermediate Row for IntTable
    IntRow::IntRow(util::ArenaVec<table::Entry> columns) : m_columns(std::move(columns)) { }

    // merge this row with a new column and return a new copy
    void IntRow::addColumn(const ast::Declaration* decl, const table::Entry& entry)
    {
        spa_assert(!this->contains(decl));
        m_columns.push_back(entry);
    }

    bool IntRow::contains(const ast::Declaration* decl) const
    {
        return std::any_of(
            m_columns.begin(), m_columns.end(), [decl](auto& ent) -> bool { return ent.getDeclaration() == decl; });
    }

    const table::Entry& IntRow::getVal(const ast::Declaration* decl) const
    {
        // assume that it exists; don't check.
        spa_assert(this->contains(decl));

        return *std::find_if(
            m_columns.begin(), m_columns.end(), [decl](auto& ent) -> bool { return ent.getDeclaration() == decl; });
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
        const auto curr_entry = std::make_pair(this->getVal(decl_a), this->getVal(decl_b));
        // curr_entries just needs to exist in one of the set of allowed entries
        return join.isAllowedEntry(curr_entry);
    }

    bool IntRow::canMerge(const IntRow& other, const TableHeaders& other_headers) const
    {
        for(const auto& decl : other_headers)
        {
            if(this->contains(decl))
            {
                const table::Entry& entry = this->getVal(decl);
                const table::Entry& other_entry = other.getVal(decl);
                // cannot merge as there are conflicting entries for the same decl
                if(entry != other_entry)
                    return false;
            }
        }
        return true;
    }

    void IntRow::mergeRow(const IntRow& other, const TableHeaders& other_headers)
    {
        spa_assert(canMerge(other, other_headers));

        size_t k = m_columns.size();
        m_columns.resize(k + other_headers.size());

        for(const auto& decl : other_headers)
        {
            if(!this->contains(decl))
                m_columns[k++] = other.getVal(decl);
        }

        m_columns.resize(k);
    }

    void IntRow::filterColumns(const TableHeaders& allowed_headers)
    {
        auto it = m_columns.begin();
        while(it != m_columns.end())
        {
            if(allowed_headers.count(it->getDeclaration()) == 0)
            {
                it = m_columns.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    const util::ArenaVec<table::Entry>& IntRow::getColumns() const
    {
        return m_columns;
    }

    size_t IntRow::size() const
    {
        return m_columns.size();
    }

    bool IntRow::operator==(const IntRow& other) const
    {
        return other.m_columns == m_columns;
#if 0
        if(this->size() != other.size())
            return false;

        for(auto& ent : m_columns)
        {
            auto decl = ent.getDeclaration();
            if(!other.contains(decl))
                return false;
            else if(other.getVal(decl) != ent)
                return false;
        }
        return true;
#endif
    }

    std::string IntRow::toString() const
    {
        std::string ret = "IntRow(\n";
        for(const auto& entry : m_columns)
        {
            ret += zpr::sprint("({}: {})\n", entry.getDeclaration()->toString(), entry.toString());
        }
        ret += ")";
        return ret;
    }


    IntTable::IntTable(util::ArenaVec<IntRow> rows, const TableHeaders& headers)
        : m_rows(std::move(rows)), m_headers(headers)
    {
        // evaluate this in an assert so it goes away during release
        spa_assert([&]() -> bool {
            for(const auto& row : m_rows)
            {
                spa_assert(row.size() == headers.size());
                for(auto& entry : row.getColumns())
                    spa_assert(headers.count(entry.getDeclaration()) > 0);
            }
            return true;
        }());
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

    void IntTable::merge(const IntTable& other)
    {
        START_BENCHMARK_TIMER(zpr::sprint("****** Time spent merging tables of {} x {}", m_rows.size(), other.size()));

        // use copy assignment to create new rows
        util::ArenaVec<IntRow> new_rows;
        new_rows.reserve(m_rows.size() * other.getRows().size());

        // m_rows should never be empty. Empty IntTable should contain an empty IntRow with no columns
        if(m_rows.empty())
        {
            util::logfmt("pql::eval::solver", "Detected IntTbl in {}. IntTbl will always be invalid", toString());
        }

        for(const auto& this_row : m_rows)
        {
            for(const auto& other_row : other.getRows())
            {
                if(this_row.canMerge(other_row, other.getHeaders()))
                {
                    IntRow new_row(this_row);
                    new_row.mergeRow(other_row, other.getHeaders());

                    new_rows.emplace_back(std::move(new_row));
                }
            }
        }

        m_rows = std::move(new_rows);

        for(const ast::Declaration* header : other.getHeaders())
            m_headers.insert(header);
    }

    void IntTable::mergeColumn(const ast::Declaration* decl, const table::Domain& domain)
    {
        if(m_headers.count(decl))
        {
            throw util::PqlException("pql::eval::solver",
                "Failed to mergeColumn decl:{} with {}. Decl already in headers", decl->toString(), toString());
        }
        // use copy assignment to create new rows
        util::ArenaVec<IntRow> new_rows;
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
                IntRow new_row(row);
                new_row.addColumn(decl, entry);
                util::logfmt("pql::eval::solver", "New row:{} from merging {} with {}", new_row.toString(),
                    row.toString(), entry.toString());

                new_rows.emplace_back(std::move(new_row));
            }
        }
        m_headers.insert(decl);
        m_rows = std::move(new_rows);
    }

    void IntTable::dedupRows()
    {
        START_BENCHMARK_TIMER(zpr::sprint("row deduplication (have {} rows)", m_rows.size()));

        // no copies required; move rows into the set (deduplicating them in the process)
        std::unordered_set<IntRow> seen {};
        for(auto& row : m_rows)
            seen.emplace(std::move(row));

        // then move them back into our list.
        m_rows.assign(std::move_iterator(seen.begin()), std::move_iterator(seen.end()));

        util::logfmt("pql::eval::solver", "Rows after deduplicating {}", toString());
    }

    const TableHeaders& IntTable::getHeaders() const
    {
        return this->m_headers;
    }

    const util::ArenaVec<IntRow>& IntTable::getRows() const
    {
        return this->m_rows;
    }

    const IntRow& IntTable::getRow(size_t i) const
    {
        return m_rows[i];
    }

    IntRow& IntTable::getRowMutable(size_t i)
    {
        return m_rows[i];
    }

    size_t IntTable::size() const
    {
        return m_rows.size();
    }

    size_t IntTable::numColumns() const
    {
        return m_headers.size();
    }

    void IntTable::filterRows(const table::Join& join)
    {
        START_BENCHMARK_TIMER(zpr::sprint("****** Time spent filtering {} rows", m_rows.size()));
        const ast::Declaration* decl_a = join.getDeclA();
        const ast::Declaration* decl_b = join.getDeclB();
        if(!(m_headers.count(decl_a) && m_headers.count(decl_b)))
        {
            util::logfmt("pql::eval::solver", "Skipping filtering {} on {} as it does not contain {} or {}",
                join.toString(), toString(), decl_a->toString(), decl_b->toString());
            return;
        }

        util::ArenaVec<IntRow> new_rows {};

        new_rows.reserve(m_rows.size());
        std::copy_if(std::move_iterator(m_rows.begin()), std::move_iterator(m_rows.end()), std::back_inserter(new_rows),
            [&](const auto& row) -> bool { return row.isAllowed(join); });

        util::logfmt(
            "pql::eval::solver", "Join(id: {}) filter tbl from {} to {}", join.getId(), m_rows.size(), new_rows.size());

        m_rows = std::move(new_rows);
    }
    void IntTable::filterColumns(const TableHeaders& allowed_columns)
    {
        for(auto& row : m_rows)
        {
            row.filterColumns(allowed_columns);
        }

        auto it = m_headers.begin();
        while(it != m_headers.end())
        {
            if(allowed_columns.count(*it) == 0)
            {
                it = m_headers.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void IntTable::mergeAndFilter(const IntTable& other, const table::Join& join)
    {
        START_BENCHMARK_TIMER(
            zpr::sprint("****** Time spent merging+filtering tables of {} x {}", m_rows.size(), other.size()));

        util::ArenaVec<IntRow> new_rows;
        new_rows.reserve(m_rows.size() * other.getRows().size());

        if(m_rows.empty())
            util::logfmt("pql::eval::solver", "Detected IntTbl in {}. IntTbl will always be invalid", toString());

        for(const auto& this_row : m_rows)
        {
            for(const auto& other_row : other.getRows())
            {
                if(this_row.canMerge(other_row, other.getHeaders()))
                {
                    IntRow new_row(this_row);
                    new_row.mergeRow(other_row, other.getHeaders());

                    if(new_row.isAllowed(join))
                        new_rows.emplace_back(std::move(new_row));
                }
            }
        }

        m_rows = std::move(new_rows);

        for(const ast::Declaration* header : other.getHeaders())
            m_headers.insert(header);
    }



    bool IntTable::empty() const
    {
        return m_rows.empty() ||
               // contains an empty row
               (m_rows.size() == 1 && m_rows.front().getColumns().empty());
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

    DepGraph::DepGraph(const TableHeaders& decls, std::vector<table::Join> joins)
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

    std::vector<TableHeaders> DepGraph::getComponents() const
    {
        std::unordered_map<int, TableHeaders> colours;
        for(const auto& [decl, colour] : m_colouring)
        {
            colours[colour].insert(decl);
        }
        std::vector<TableHeaders> ret;
        for(const auto& [_, comp] : colours)
        {
            ret.emplace_back(comp);
        }
        return ret;
    }

    static TableHeaders mergeAndCopySet(TableHeaders a, const TableHeaders& b)
    {
        TableHeaders ret(std::move(a));
        for(const ast::Declaration* decl : b)
        {
            ret.insert(decl);
        }
        return ret;
    }

    std::vector<std::vector<const ast::Declaration*>> Solver::sort_components(
        const std::vector<TableHeaders>& components) const
    {
        std::vector<std::vector<const ast::Declaration*>> ret;
        for(const auto& component : components)
        {
            std::vector<std::pair<int, const ast::Declaration*>> size_decls;
            for(const auto decl : component)
            {
                // All decl should belong to a table
                spa_assert(has_table(decl));
                size_t table_i = get_table_index(decl);
                size_decls.emplace_back(m_int_tables[table_i].size(), decl);
            }
            // sort smallest IntTable first
            std::sort(size_decls.begin(), size_decls.end());

            std::vector<const ast::Declaration*> sorted_decl(size_decls.size());

            for(size_t i = 0; i < size_decls.size(); i++)
            {
                sorted_decl[i] = size_decls[i].second;
            }

            ret.emplace_back(std::move(sorted_decl));
        }
        return ret;
    };
    Solver::Solver(const std::vector<table::Join>& joins,
        std::unordered_map<const ast::Declaration*, table::Domain> domains, const TableHeaders& return_decls,
        const TableHeaders& select_decls)
        : m_domains(std::move(domains)), m_joins(joins), m_return_decls(return_decls), m_int_tables(),
          m_decl_components(), m_dep_graph(mergeAndCopySet(return_decls, select_decls), joins)
    {
        START_BENCHMARK_TIMER("Solver constructor");
        trim(return_decls);
        trim(select_decls);
        // all declaration should start as table initially
        for(const ast::Declaration* decl : return_decls)
        {
            IntTable tbl {};
            if(m_domains.count(decl) == 0)
                throw util::PqlException("pql::eval::solver", "{} does not have any domain", decl->toString());

            tbl.mergeColumn(decl, m_domains.find(decl)->second);
            m_int_tables.push_back(std::move(tbl));

            util::logfmt("pql::eval::solver", "Adding {} to m_int_tables", tbl.toString());
        }
        // all declaration should start as table initially
        for(const ast::Declaration* decl : select_decls)
        {
            IntTable tbl {};
            if(m_domains.count(decl) == 0)
                throw util::PqlException("pql::eval::solver", "{} does not have any domain", decl->toString());

            if(has_table(decl))
            {
                // IntTbl already initialized as decl is also a ret decl.
                continue;
            }
            tbl.mergeColumn(decl, m_domains.find(decl)->second);
            m_int_tables.push_back(std::move(tbl));

            util::logfmt("pql::eval::solver", "Adding {} to m_int_tables", tbl.toString());
        }
        // only sort component after forming the IntTable
        m_decl_components = sort_components(m_dep_graph.getComponents());
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
                "Failed to trim {} with {}. Declaration's domain not initialised", decl->toString(), join.toString());

        const std::unordered_set<table::Entry>& domain = m_domains.find(decl)->second;
        auto& join_allowed_entries = join.getAllowedEntries();
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
        m_domains[decl] = std::move(entry_set_intersect);

        // remove allowed entries to only contain trimmed entries
        auto join_it = join_allowed_entries.begin();
        while(join_it != join_allowed_entries.end())
        {
            const table::Entry& entry = join_it->first.getDeclaration() == decl ? join_it->first : join_it->second;
            // remove allowed entries that are not in intersect
            if(m_domains[decl].count(entry) == 0)
            {
                util::logfmt("pql::eval::solver", "Removing {} from {}.", entry.toString(), join.toString());
                join_it = join_allowed_entries.erase(join_it);
            }
            else
            {
                join_it++;
            }
        }
    }

    void Solver::trim(const TableHeaders& decls)
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
        TableHeaders processed_decl;
        std::vector<IntTable> new_int_tables;
        std::unordered_set<int> processed_join;

        for(std::vector<const ast::Declaration*>& component : m_decl_components)
        {
            IntTable new_table;
            // A component should never be empty
            spa_assert(!component.empty());
            util::logfmt("pql::eval::solver", "Merging component {}", [&]() -> std::string {
                std::string log = "{";
                for(const ast::Declaration* decl : component)
                    log += decl->toString() + ", ";
                return log + "}";
            }());

            std::sort(component.begin(), component.end(), [&](auto& a, auto& b) -> bool {
                return m_int_tables[get_table_index(a)].size() < m_int_tables[get_table_index(b)].size();
            });

            for(const ast::Declaration* decl : component)
            {
                IntTable& prev_table = m_int_tables[get_table_index(decl)];
                // merge to new table if it has not been processed
                if(new_table.getHeaders().count(decl) == 0)
                {
                    util::logfmt("pql::eval::solver", "merging decl {} from {} into {}", decl->toString(),
                        prev_table.toString(), new_table.toString());
                    new_table.merge(prev_table);
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

                        new_table.mergeAndFilter(other_prev_table, join);
                    }
                    else
                    {
                        new_table.filterRows(join);
                    }

                    processed_join.insert(join.getId());
                    // If a table is empty, there will never be a valid assignment and we can terminate early
                    if(new_table.size() == 0)
                        break;
                }
            }

            util::logfmt("pql::eval::solver", "New final merged table for component {}", new_table.toString());

            /*
                there are two things to note here:
                1. if the table has *no rows*, we *MUST* push it to the list of tables. this is because
                    we use the "has no rows" condition to know whether a query succeeded or failed.

                2. however, if, after filtering away unnecessary columns (decls), the table is left with
                    *no columns*, we cannot add it to the list of tables, since it (by definition) would
                    have no rows.

                    even though it has no rows, it *HAD* rows before we yeeted all the columns, so that means
                    that the query should not fail (or at least, should not fail because of this group of decls)
            */

            if(new_table.size() > 0)
            {
                new_table.filterColumns(m_return_decls);
                if(new_table.numColumns() == 0)
                    continue;
            }

            new_table.dedupRows();
            new_int_tables.push_back(std::move(new_table));
            // If a table is empty, there will never be a valid assignment and we can terminate early
            if(new_int_tables.back().size() == 0)
                break;
        }

        m_int_tables = std::move(new_int_tables);
        util::logfmt("pql::eval::solver", "Solver after preprocessing {}", toString());
    }

    bool Solver::isValid() const
    {
        for(const IntTable& tbl : m_int_tables)
        {
            if(tbl.empty())
                return false;
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
        spa_assert(!m_return_decls.empty());

        auto return_decls = std::vector<const ast::Declaration*>(m_return_decls.begin(), m_return_decls.end());
        std::sort(return_decls.begin(), return_decls.end(), [&](auto& a, auto& b) -> bool {
            return m_int_tables[get_table_index(a)].size() < m_int_tables[get_table_index(b)].size();
        });

        for(const ast::Declaration* decl : return_decls)
        {
            util::logfmt("pql::eval::solver", "Handling return decl {}", decl->toString());
            // already added into table by another decl in the same component
            if(ret_table.getHeaders().count(decl))
                continue;

            IntTable& decl_int_table = m_int_tables[get_table_index(decl)];

            util::logfmt(
                "pql::eval::solver", "Merging table {} to {}", ret_table.toString(), decl_int_table.toString());

            ret_table.merge(decl_int_table);
        }
        util::logfmt("pql::eval::solver", "Return table: {}", ret_table.toString());
        return ret_table;
    }
}
