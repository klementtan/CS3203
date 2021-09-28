#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include <zpr.h>
#include "simple/parser.h"
#include "design_extractor.h"
#include "pkb.h"
#include "util.h"
using namespace simple::parser;
using namespace pkb;

TEST_CASE("Populate PKB")
{
    SECTION("Cyclic calls")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
            procedure C {
	            call A;
            }
        )";

        auto prog = parseProgram(in);
        REQUIRE_THROWS_WITH(DesignExtractor(std::move(prog)).run(), "illegal cyclic/recursive call");
    }

    SECTION("cyclic/recursive call")
    {
        constexpr const auto in = R"(
            procedure B {
	            call B;
            }
        )";

        auto prog = parseProgram(in);
        REQUIRE_THROWS_WITH(DesignExtractor(std::move(prog)).run(), "illegal cyclic/recursive call");
    }

    SECTION("Recursive calls in disjoint graphs")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
            procedure C {
	            call D;
            }
            procedure G {
	            call G;
            }
            procedure Z {
	            call X;
            }
            procedure X {
	            call V;
            }
            procedure V {
	            a = 1;
            }
            procedure D {
	            a = 1;
            }
        )";

        auto prog = parseProgram(in);
        REQUIRE_THROWS_WITH(DesignExtractor(std::move(prog)).run(), "illegal cyclic/recursive call");
    }

    SECTION("Call to non-existent procedure")
    {
        constexpr const auto in = R"(
            procedure A {
	            call B;
            }
            procedure B {
	            call C;
            }
        )";
        auto prog = parseProgram(in);
        REQUIRE_THROWS_WITH(DesignExtractor(std::move(prog)).run(), "call to undefined procedure 'C'");
    }
}
