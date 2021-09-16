// runner.cpp

#define CATCH_CONFIG_FAST_COMPILE 1
#include "catch.hpp"

#include "runner.h"
#include "exceptions.h"
#include "simple/parser.h"

bool Test::run() const
{
    std::string actual {};
    std::string expected = this->expected.str();

    bool ok = false;
    try
    {
        if(this->whole_prog)
        {
            auto res = simple::parser::parseProgram(this->input);
            actual = res->toString(/* compact: */ true);

            delete res;
        }
        else
        {
            auto res = simple::parser::parseExpression(this->input);
            actual = res->toString();

            delete res;
        }
        ok = true;
    }
    catch(const util::Exception& e)
    {
        actual = e.what();
        ok = false;
    }

    if(ok != this->should_pass)
    {
        zpr::fprintln(stderr, "test case failed: expected {}, got {}", this->should_pass ? "success" : "failure",
            ok ? "success" : "failure");
        zpr::fprintln(stderr, "input:\n{}", this->input);
        zpr::fprintln(stderr, "result:\n{}", actual);

        if(actual != expected)
            zpr::fprintln(stderr, "expected:\n{}", expected);

        return false;
    }
    else if(actual != expected)
    {
        zpr::fprintln(stderr, "test case failed:");
        zpr::fprintln(stderr, "input:\n{}", this->input);
        zpr::fprintln(stderr, "result:\n{}", actual);
        zpr::fprintln(stderr, "expected:\n{}", expected);
        return false;
    }
    else
    {
        return true;
    }
}
