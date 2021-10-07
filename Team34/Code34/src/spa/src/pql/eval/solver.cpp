#include "pql/eval/solver.h"
#include "exceptions.h"
#include "zpr.h"
#include "util.h"

namespace pql::eval::solver
{
    // Intermediate Row for IntTable
    IntRow::IntRow(const std::unordered_map<const ast::Declaration*, table::Entry>& columns) : m_columns(columns) { }
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
        const std::pair<table::Entry, table::Entry> curr_entries =
            std::make_pair(m_columns.find(decl_a)->second, m_columns.find(decl_b)->second);
        // curr_entries just needs to exist in one of the set of allowed entries
        return join.getAllowedEntries().count(curr_entries) > 0;
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
        if(!canMerge(other))
            throw util::PqlException("pql::eval::solver",
                "Cannot merge {} with {} as there are conflicting entries for the same decl", toString(),
                other.toString());
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


    IntTable::IntTable(const std::vector<IntRow>& rows, const std::unordered_set<const ast::Declaration*>& headers)
        : m_rows(rows), m_headers(headers)
    {
        // TOOD: Remove this sanity check during release
        for(const auto& row : m_rows)
        {
          if (row.getHeaders() != m_headers)
          {
            throw util::PqlException("pql::eval::solver", "Initialised {} with mismatched rows and headers", toString());
          }

        }
    };

    IntTable::IntTable() : m_rows(), m_headers() {};

    bool IntTable::contains(const ast::Declaration* declaration)
    {
        return m_headers.count(declaration);
    }

    IntTable IntTable::merge(const IntTable& other)
    {
        // use copy assignment to create new rows
        std::vector<IntRow> new_rows = m_rows;
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
    };

    IntTable IntTable::mergeColumn(const ast::Declaration* decl, const table::Domain& domain) const
    {
        if(m_headers.count(decl))
        {
            util::PqlException("pql::eval::solver", "Failed to mergeColumn decl:{} with {}. Decl already in headers",
                decl->toString(), toString());
        }
        // use copy assignment to create new rows
        std::vector<IntRow> new_rows = m_rows;
        for(const auto& row : m_rows)
        {
            for(const auto& entry : domain)
            {
                if(entry.getDeclaration() != decl)
                    util::PqlException("pql::eval::solver",
                        "Failed to merge column to {} due to conflicting decl:{} and provided entry: {}", toString(),
                        decl->toString(), entry.getDeclaration()->toString());
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
    };

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
        std::unordered_set<std::pair<table::Entry, table::Entry>> allowed_entries = join.getAllowedEntries();
        auto it = m_rows.begin();
        while(it != m_rows.end())
        {
            table::Entry entry_a = it->getVal(decl_a);
            table::Entry entry_b = it->getVal(decl_b);
            if(allowed_entries.count({ entry_a, entry_b }))
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
    bool IntTable::empty()
    {
        return m_rows.empty();
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
}
