// cfg_bip.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "design_extractor.h"
#include "simple/parser.h"
#include "pkb.h"
#include "util.h"
#include <iostream>
#include <zpr.h>
using namespace simple::parser;
using namespace pkb;

constexpr const auto sample_source_A = R"(
procedure Bill {
      x = 5;
      call Mary;
      y = x + 6;
      call John;
      z = x * y + 2; }

procedure Mary {
      y = x * 3;
      call John;
      z = x + y; }

procedure John {
      if (i > 0) then {
              x = x + z; }
      else {
              y = x * y; } }

)";
static auto kb1 = DesignExtractor(parseProgram(sample_source_A)).run();
static auto cfg1 = kb1 -> getCFG();
