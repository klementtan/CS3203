#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/solver.h"

std::vector<std::unique_ptr<pql::ast::Declaration>> generate_decl(int count, int start_i)
{
    std::vector<std::unique_ptr<pql::ast::Declaration>> ret(count);
    for(int i = 0; i < count; i++)
    {
        ret[i] = std::make_unique<pql::ast::Declaration>(
            pql::ast::Declaration { "a" + to_string(start_i + i), pql::ast::DESIGN_ENT::ASSIGN });
    }
    return ret;
}

std::vector<pql::eval::solver::IntRow> generate_rows(int count, std::vector<pql::ast::Declaration*> decls)
{
    std::vector<pql::eval::solver::IntRow> rows(count);
    for(int i = 0; i < count; i++)
    {
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns;
        for(pql::ast::Declaration* decl : decls)
        {
            columns[decl] = pql::eval::table::Entry(decl, i);
        }
        rows[i] = pql::eval::solver::IntRow(columns);
    }
    return rows;
}

TEST_CASE("IntRow")
{
    SECTION("addColumn getVal getHeaders")
    {
        std::unique_ptr<pql::ast::Declaration> decl =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry = pql::eval::table::Entry(decl.get(), 1);
        pql::eval::solver::IntRow row;
        pql::eval::solver::IntRow result(row);
        result.addColumn(decl.get(), entry);
        REQUIRE(result.getVal(decl.get()) == entry);
        REQUIRE(*(result.getHeaders().begin()) == decl.get());
        // Should not change original row
        REQUIRE(row.getHeaders().empty());
    }
    SECTION("contains")
    {
        std::unique_ptr<pql::ast::Declaration> decl =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry = pql::eval::table::Entry(decl.get(), 1);
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns = { { decl.get(), entry } };
        pql::eval::solver::IntRow row(columns);
        REQUIRE(row.contains(decl.get()));
    }
    SECTION("isAllowed")
    {
        std::unique_ptr<pql::ast::Declaration> decl1 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a1", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry1 = pql::eval::table::Entry(decl1.get(), 1);
        std::unique_ptr<pql::ast::Declaration> decl2 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a2", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry2 = pql::eval::table::Entry(decl1.get(), 2);
        pql::eval::table::Join valid_join(decl1.get(), decl2.get(), { { entry1, entry2 } });
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns = { { decl1.get(), entry1 },
            { decl2.get(), entry2 } };
        pql::eval::solver::IntRow row(columns);
        REQUIRE(row.isAllowed(valid_join));

        pql::eval::table::Join invalid_join(
            decl1.get(), decl2.get(), { { entry1, pql::eval::table::Entry(decl2.get(), 420) } });
        REQUIRE_FALSE(row.isAllowed(invalid_join));
    }
    SECTION("canMerge")
    {
        std::unique_ptr<pql::ast::Declaration> decl1 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a1", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry1 = pql::eval::table::Entry(decl1.get(), 1);
        std::unique_ptr<pql::ast::Declaration> decl2 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a2", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry2 = pql::eval::table::Entry(decl1.get(), 2);
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns1 = { { decl1.get(),
            entry1 } };
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns2 = { { decl2.get(),
            entry2 } };
        pql::eval::solver::IntRow row1(columns1);
        pql::eval::solver::IntRow row2(columns2);
        // Disjoint rows
        REQUIRE(row1.canMerge(row2));

        // Overlapping rows
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns3 = { { decl1.get(), entry1 },
            { decl2.get(), entry2 } };
        pql::eval::solver::IntRow row3(columns3);
        REQUIRE(row1.canMerge(row3));

        // Conflicting columns in rows
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns4 = {
            { decl1.get(), pql::eval::table::Entry(decl1.get(), 420) }, { decl2.get(), entry2 }
        };
        pql::eval::solver::IntRow row4(columns4);
        REQUIRE_FALSE(row1.canMerge(row4));
    }
    SECTION("mergeRow")
    {
        std::unique_ptr<pql::ast::Declaration> decl1 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a1", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry1 = pql::eval::table::Entry(decl1.get(), 1);
        std::unique_ptr<pql::ast::Declaration> decl2 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a2", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry2 = pql::eval::table::Entry(decl1.get(), 2);
        std::unique_ptr<pql::ast::Declaration> decl3 =
            std::make_unique<pql::ast::Declaration>(pql::ast::Declaration { "a3", pql::ast::DESIGN_ENT::ASSIGN });
        pql::eval::table::Entry entry3 = pql::eval::table::Entry(decl1.get(), 3);
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns1 = { { decl1.get(),
            entry1 } };
        std::unordered_map<const pql::ast::Declaration*, pql::eval::table::Entry> columns2 = { { decl2.get(), entry2 },
            { decl3.get(), entry3 } };
        pql::eval::solver::IntRow row1(columns1);
        pql::eval::solver::IntRow row2(columns2);
        pql::eval::solver::IntRow merged_row(row1);
        merged_row.mergeRow(row2);
        // Disjoint rows
        REQUIRE(merged_row.getVal(decl1.get()) == entry1);
        REQUIRE(merged_row.getVal(decl2.get()) == entry2);
        REQUIRE(merged_row.getVal(decl3.get()) == entry3);
    }
}
TEST_CASE("IntTable")
{
    SECTION("contains")
    {
        std::vector<std::unique_ptr<pql::ast::Declaration>> decls = generate_decl(5, 0);
        std::vector<pql::ast::Declaration*> decl_observers(decls.size());
        for(size_t i = 0; i < decls.size(); i++)
            decl_observers[i] = decls[i].get();
        std::vector<pql::eval::solver::IntRow> rows = generate_rows(5, decl_observers);
        pql::eval::solver::IntTable tbl(
            rows, std::unordered_set<const pql::ast::Declaration*>(decl_observers.begin(), decl_observers.end()));
        for(const auto& decl : decl_observers)
        {
            REQUIRE(tbl.contains(decl));
        }
    }

    SECTION("merge")
    {
        pql::eval::solver::IntTable tbl0;
        std::vector<std::unique_ptr<pql::ast::Declaration>> decls1 = generate_decl(5, 0);
        std::vector<pql::ast::Declaration*> decl_observers1(decls1.size());
        for(size_t i = 0; i < decls1.size(); i++)
            decl_observers1[i] = decls1[i].get();
        std::vector<pql::eval::solver::IntRow> rows1 = generate_rows(2, decl_observers1);
        pql::eval::solver::IntTable tbl1(
            rows1, std::unordered_set<const pql::ast::Declaration*>(decl_observers1.begin(), decl_observers1.end()));
        pql::eval::solver::IntTable merged_tbl1(tbl0);
        merged_tbl1.merge(tbl1);
        REQUIRE(merged_tbl1.getRows().size() == 2);

        std::vector<std::unique_ptr<pql::ast::Declaration>> decls2 = generate_decl(5, 5);
        std::vector<pql::ast::Declaration*> decl_observers2(decls2.size());
        for(size_t i = 0; i < decls2.size(); i++)
            decl_observers2[i] = decls2[i].get();
        std::vector<pql::eval::solver::IntRow> rows2 = generate_rows(2, decl_observers2);
        pql::eval::solver::IntTable tbl2(
            rows2, std::unordered_set<const pql::ast::Declaration*>(decl_observers2.begin(), decl_observers2.end()));
        pql::eval::solver::IntTable merged_tbl2(tbl1);
        merged_tbl2.merge(tbl2);
        // cross product of table of sizes 2 x 2
        REQUIRE(merged_tbl2.getRows().size() == 4);
    }

    SECTION("mergeColumn")
    {
        std::unique_ptr<pql::ast::Declaration> decl = std::move(generate_decl(1, 0).front());
        std::unordered_set<pql::eval::table::Entry> entries = { pql::eval::table::Entry(decl.get(), 1),
            pql::eval::table::Entry(decl.get(), 2), pql::eval::table::Entry(decl.get(), 3),
            pql::eval::table::Entry(decl.get(), 4) };
        pql::eval::solver::IntTable tbl0;
        pql::eval::solver::IntTable merged_tbl(tbl0);
        merged_tbl.mergeColumn(decl.get(), entries);
        REQUIRE(merged_tbl.getRows().size() == entries.size());
        for(const pql::eval::solver::IntRow& row : merged_tbl.getRows())
        {
            REQUIRE(row.getHeaders().size() == 1);
            REQUIRE(entries.count(row.getVal(decl.get())) == 1);
        }
    }

    SECTION("filterRows")
    {
        std::vector<std::unique_ptr<pql::ast::Declaration>> decls1 = generate_decl(5, 0);
        std::vector<pql::ast::Declaration*> decl_observers1(decls1.size());
        for(size_t i = 0; i < decls1.size(); i++)
            decl_observers1[i] = decls1[i].get();
        std::vector<pql::eval::solver::IntRow> rows1 = generate_rows(10, decl_observers1);
        pql::eval::solver::IntTable tbl1(
            rows1, std::unordered_set<const pql::ast::Declaration*>(decl_observers1.begin(), decl_observers1.end()));
        pql::ast::Declaration* decl_a = decl_observers1[0];
        pql::ast::Declaration* decl_b = decl_observers1[1];
        std::unordered_set<std::pair<pql::eval::table::Entry, pql::eval::table::Entry>> allowed_entries = {
            { rows1[0].getVal(decl_a), rows1[0].getVal(decl_b) },
            { rows1[3].getVal(decl_a), rows1[3].getVal(decl_b) },
        };
        pql::eval::table::Join join(decl_a, decl_b, allowed_entries);
        tbl1.filterRows(join);
        REQUIRE(tbl1.getRows().size() == 2);
        for(const auto& row : tbl1.getRows())
        {
            pql::eval::table::Entry a_val = row.getVal(decl_a);
            pql::eval::table::Entry b_val = row.getVal(decl_b);
            bool is_a_valid = (a_val == rows1[0].getVal(decl_a)) || (a_val == rows1[3].getVal(decl_a));
            bool is_b_valid = (b_val == rows1[0].getVal(decl_b)) || (b_val == rows1[3].getVal(decl_b));
            REQUIRE(is_a_valid);
            REQUIRE(is_b_valid);
        }
    }
}
