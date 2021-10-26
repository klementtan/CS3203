// solver.h
//
// table.h Solver

#pragma once
#include <pql/parser/ast.h>
#include <pql/eval/table.h>

namespace pql::eval::solver
{
    // Intermediate Row for IntTable
    class IntRow
    {
    private:
        // mapping of declaration and the index it belongs to
        std::unordered_map<const ast::Declaration*, table::Entry> m_columns;

    public:
        explicit IntRow(std::unordered_map<const ast::Declaration*, table::Entry> columns);
        IntRow();
        // merge this row with a new column and return a new copy
        void addColumn(const ast::Declaration* decl, const table::Entry& entry);
        [[nodiscard]] bool canMerge(const IntRow& other) const;
        // merge other row into current row
        void mergeRow(const IntRow& other);
        [[nodiscard]] std::unordered_set<const ast::Declaration*> getHeaders() const;
        // Remove columns that are not in the allowed headers
        void filterColumns(const std::unordered_set<const ast::Declaration*>& allowed_headers);
        table::Entry getVal(const ast::Declaration* decl) const;
        bool contains(const ast::Declaration* decl) const;
        int size() const;
        const std::unordered_map<const ast::Declaration*, table::Entry>& getColumns() const;
        // check columns in the row exist in one of the allowed joins
        [[nodiscard]] bool isAllowed(const table::Join& join) const;
        [[nodiscard]] std::string toString() const;

        bool operator==(const IntRow& other) const;
    };

    // Intermediate Table for solver
    class IntTable
    {
    private:
        std::vector<IntRow> m_rows;
        // header for rows
        std::unordered_set<const ast::Declaration*> m_headers;

    public:
        IntTable(std::vector<IntRow> rows, const std::unordered_set<const ast::Declaration*>& headers);
        IntTable();
        bool contains(const ast::Declaration* declaration);
        // Performs cross product on the rows
        void merge(const IntTable& other);
        // Performs cross product on the Domain and return a new IntTable (O(N^2))
        void mergeColumn(const ast::Declaration* decl, const table::Domain& domain);
        // Remove columns that are not in the allowed headers
        void filterColumns(const std::unordered_set<const ast::Declaration*>& allowed_headers);
        [[nodiscard]] std::unordered_set<const ast::Declaration*> getHeaders() const;
        [[nodiscard]] const std::vector<IntRow>& getRows() const;
        void filterRows(const table::Join& join);
        void dedupRows();
        const IntRow& getRow(int i) const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] int size() const;
        [[nodiscard]] std::string toString() const;
    };

    // Dependency graph for join conditions
    class DepGraph
    {
    private:
        std::vector<table::Join> m_joins;
        std::unordered_map<const ast::Declaration*, int> m_colouring;
        std::unordered_map<const ast::Declaration*, std::unordered_set<const ast::Declaration*>> m_graph;
        void colour_node(const ast::Declaration* s, int colour);

    public:
        DepGraph(const std::unordered_set<const ast::Declaration*>& decls, std::vector<table::Join> joins);

        [[nodiscard]] std::vector<std::unordered_set<const ast::Declaration*>> getComponents() const;
        [[nodiscard]] std::string toString() const;
    };

    // Solver
    class Solver
    {
    private:
        std::unordered_map<const ast::Declaration*, table::Domain> m_domains;
        std::vector<table::Join> m_joins;
        std::unordered_set<const ast::Declaration*> m_return_decls;
        std::vector<IntTable> m_int_tables;
        std::vector<std::vector<const ast::Declaration*>> m_decl_components;
        DepGraph m_dep_graph;

        std::vector<table::Join> get_joins(const ast::Declaration* decl) const;

        // trim the domain and allowed entries that are associated with decls
        //
        // all allowed entries should be in the domain and all domain should be in the allowed entries
        void trim(const std::unordered_set<const ast::Declaration*>& decls);
        void trim_helper(const ast::Declaration* decls, table::Join& join);
        size_t get_table_index(const ast::Declaration* decl) const;
        // preprocess by joining tables based on the Joins
        void preprocess_int_table();
        bool has_table(const ast::Declaration* decl) const;
        std::vector<std::vector<const ast::Declaration*>> sort_components(
            const std::vector<std::unordered_set<const ast::Declaration*>>& components) const;

    public:
        Solver(const std::vector<table::Join>& joins,
            std::unordered_map<const ast::Declaration*, table::Domain> domains,
            const std::unordered_set<const ast::Declaration*>& return_decls,
            const std::unordered_set<const ast::Declaration*>& select_decls);
        [[nodiscard]] IntTable getRetTbl();
        [[nodiscard]] bool isValid() const;
        [[nodiscard]] std::string toString() const;
    };

}

namespace std
{
    template <>
    struct hash<pql::eval::solver::IntRow>
    {
        size_t operator()(const pql::eval::solver::IntRow& r) const
        {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            for(const auto& [decl, e] : r.getColumns())
            {
                res += std::hash<const pql::ast::Declaration*>()(decl);
                if(e.getType() != pql::eval::table::EntryType::kStmt)
                    res += std::hash<string>()(e.getVal());
                if(e.getType() == pql::eval::table::EntryType::kStmt)
                    res += std::hash<simple::ast::StatementNum>()(e.getStmtNum());
                res += std::hash<pql::ast::Declaration>()(*e.getDeclaration());
                res += std::hash<pql::eval::table::EntryType>()(e.getType());
            }
            return res;
        }
    };
}
