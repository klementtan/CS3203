// test_modifies.cpp

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include "runner.h"

#include "pql/eval/evaluator.h"
#include "pql/parser/parser.h"
#include "simple/parser.h"
#include "pkb.h"

constexpr const auto prog_1 = R"(
    procedure main {
      flag = 0;
      call computeCentroid;
      call printResults;
    }
    procedure readPoint {
        read x;
        read y;
    }
    procedure printResults {
        print flag;
        print cenX;
        print cenY;
        print normSq;
    }
    procedure computeCentroid {
        count = 0;
        cenX = 0;
        cenY = 0;
        call readPoint;
        while ((x != 0) && (y != 0)) {
            count = count + 1;
            cenX = cenX + x;
            cenY = cenY + y;
            call readPoint;
        }
        if (count == 0) then {
            flag = 1;
        } else {
            cenX = cenX / count;
            cenY = cenY / count;
        }
        normSq = cenX * cenX + cenY * cenY;
    }
)";

constexpr const auto prog_2 = R"(
procedure foo {
    read x;
    read y;
    z = 10;
}
procedure bar {
    print a;
    print b;
    while(c == 0) {
        print d;
    }
}
)";

constexpr const auto prog_3 = R"(
procedure foo {
    print x;
    print y;
}
procedure bar {
    print a;
    print b;
}
)";


TEST_CASE("ModifiesP(DeclaredEnt, DeclaredEnt)")
{
    TEST_OK(
        prog_1, "procedure p; variable v; Select p such that Modifies(p, v)", "main", "readPoint", "computeCentroid");

    TEST_OK(prog_2, "procedure p; variable v; Select p such that Modifies(p, v)", "foo");
    TEST_OK(prog_2, "procedure p; variable v; Select v such that Modifies(p, v)", "x", "y", "z");

    TEST_EMPTY(prog_3, "procedure p; variable v; Select p such that Modifies(p, v)");
    TEST_EMPTY(prog_3, "procedure p; variable v; Select v such that Modifies(p, v)");
}

TEST_CASE("ModifiesP(DeclaredEnt, EntName)")
{
    TEST_OK(prog_1, "procedure p; Select p such that Modifies(p, \"flag\")", "main", "computeCentroid");
}

TEST_CASE("ModifiesP(DeclaredEnt, _)")
{
    TEST_OK(prog_1, "procedure p; Select p such that Modifies(p, _)", "main", "readPoint", "computeCentroid");
}

TEST_CASE("ModifiesP(EntName, DeclaredEnt)")
{
    TEST_OK(prog_1, "variable v; Select v such that Modifies(\"main\", v)", "x", "y", "cenX", "cenY", "count", "flag",
        "normSq");
}

TEST_CASE("ModifiesP(EntName, EntName)")
{
    TEST_OK(prog_1, "variable v; Select v such that Modifies(\"main\", \"flag\")", "x", "y", "cenX", "cenY", "count",
        "flag", "normSq");

    // ModifiesP(modifier:EntName(name:readPoint), ent:EntName(name:flag)) always
    // evaluate to false. readPoint does not modify flag.
    TEST_EMPTY(prog_1, "variable v; Select v such that Modifies(\"readPoint\", \"flag\")");
}

TEST_CASE("ModifiesP(EntName, _)")
{
    TEST_OK(prog_1, "variable v; Select v such that Modifies(\"readPoint\", _)", "x", "y", "cenX", "cenY", "count",
        "flag", "normSq");

    // ModifiesP(modifier:EntName(name:printResults), ent:AllEnt(name: _)) always
    // evaluate to false. printResults does not modify any variable.
    TEST_EMPTY(prog_1, "variable v; Select v such that Modifies(\"printResults\", _)");
}






TEST_CASE("ModifiesS(DeclaredStmt, DeclaredEnt)")
{
    TEST_OK(
        prog_1, "assign a; variable v; Select a such that Modifies(a, v)", 1, 10, 11, 12, 15, 16, 17, 20, 21, 22, 23);

    TEST_EMPTY(prog_2, "print p; variable v; Select p such that Modifies(p, v)");
    TEST_EMPTY(prog_2, "print p; variable v; Select v such that Modifies(p, v)");

    TEST_EMPTY(prog_3, "print p; variable v; Select p such that Modifies(p, v)");
    TEST_EMPTY(prog_3, "print p; variable v; Select v such that Modifies(p, v)");
}

TEST_CASE("ModifiesS(DeclaredStmt, EntName)")
{
    TEST_OK(prog_1, "assign a; variable v; Select a such that Modifies(a, \"flag\")", 1, 20);
}

TEST_CASE("ModifiesS(DeclaredStmt, _)")
{
    TEST_OK(prog_1, "call c; Select c such that Modifies(c, _)", 2, 13, 18);
}

TEST_CASE("ModifiesS(StmtId, DeclaredStmt)")
{
    TEST_OK(prog_1, "variable v; Select v such that Modifies(23, v)", "normSq");
}

TEST_CASE("ModifiesS(StmtId, _)")
{
    TEST_OK(
        prog_1, "variable v; Select v such that Modifies(23, _)", "normSq", "x", "y", "flag", "cenX", "cenY", "count");

    // ModifiesS(modifier:StmtId(id:3), ent:AnyEnt(name: _)) always evaluate
    // to false. StatementNum 3 does not modify any variable.
    TEST_EMPTY(prog_1, "variable v; Select v such that Modifies(3, _)");
}

TEST_CASE("ModifiesS(StmtId, EntName)")
{
    TEST_OK(prog_1, "variable v; Select v such that Modifies(23, \"normSq\")", "normSq", "x", "y", "flag", "cenX",
        "cenY", "count");

    // StatementNum: ModifiesS(modifier:StmtId(id:23), ent:EntName(name:x))
    // always evaluate to false. 23 does not modify x.
    TEST_EMPTY(prog_1, "variable v; Select v such that Modifies(23, \"x\")");
}
