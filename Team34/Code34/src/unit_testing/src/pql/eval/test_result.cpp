// test_result.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

constexpr const auto prog_1 = R"(
      procedure First {
      read x;
      read z;
      call Second; }
  
      procedure Second {
          x = 0;
          i = 5;
          while (i!=0) {
              x = x + 2*y;
              call Third;
              i = i - 1; }
          if (x==1) then {
              x = x+1; }
          else {
              z = 1; }
          z = z + x + i;
          y = z + 2;
          x = x * y + z; }
  
      procedure Third {
          z = 5;
          v = z;
          print v; }

)";

constexpr const auto prog_2 = R"(
procedure p {
    if (1 == 2) then {
        while (3 == 4) {
            x = 1; } }
    else {
        if (x == 1) then {
            while (y == x) {
                z = 2; } }
        else {
            y = 3; } } }
)";

TEST_CASE("Select BOOLEAN")
{
    TEST_OK(prog_1, R"(assign a; Select BOOLEAN)", "TRUE");
    TEST_OK(prog_1, R"(assign a; Select BOOLEAN pattern a(_,"foooobar"))", "FALSE");
}

TEST_CASE("Attribute Name")
{
    // Should return the corresponding AttrRef for the allowed Declaration
    // procName
    TEST_OK(prog_1, R"(procedure p; Select p.procName)", "First", "Second", "Third");
    TEST_OK(prog_1, R"(call c; Select c.procName)", "Second", "Third");

    // varName
    TEST_OK(prog_1, R"(variable v; Select v.varName)", "x", "z", "i", "y", "v");
    TEST_OK(prog_1, R"(read r; Select r.varName)", "x", "z");
    TEST_OK(prog_1, R"(print p; Select p.varName)", "v");

    // value
    TEST_OK(prog_1, R"(constant c; Select c.value)", "0", "5", "2", "1");

    // stmt#
    TEST_OK(prog_1, R"(stmt s; Select s.stmt#)", "3", "4", "1", "5", "6", "7", "8", "10", "12", "13", "18", "2", "14",
        "9", "11", "16", "15", "17");
    TEST_OK(prog_1, R"(read r; Select r.stmt#)", "1", "2");
    TEST_OK(prog_1, R"(print p; Select p.stmt#)", "18");
    TEST_OK(prog_1, R"(call c; Select c.stmt#)", "3", "8");
    TEST_OK(prog_1, R"(while w; Select w.stmt#)", "6");
    TEST_OK(prog_1, R"(if ifs; Select ifs.stmt#)", "10");
    TEST_OK(prog_1, R"(assign a; Select a.stmt#)", "5", "7", "4", "13", "15", "12", "17", "9", "16", "11", "14");
}

TEST_CASE("Tuple Result Clause")
{
    TEST_OK(prog_1, R"(procedure p,q; Select <p,q> such that Calls(p,q))", "First Second", "Second Third");
    // Dependent declaration
    TEST_OK(prog_1, R"(stmt s1; while w; Select <s1,w> such that Follows(s1,w))", "5 6");

    // Non dependent declaration
    TEST_OK(prog_1, R"(stmt s1; while w; Select <s1,w> such that Follows(s1,w))", "5 6");

    TEST_OK(prog_2, R"(if ifs; variable v; Select <ifs, v> pattern ifs(v,_,_))", "4 x");
    TEST_OK(prog_1, R"(call c; read r; Select <c, r>)", "3 1", "3 2", "8 1", "8 2");
}
