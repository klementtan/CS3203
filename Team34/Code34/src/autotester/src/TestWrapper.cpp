// TestWrapper.cpp

#include <cstdlib>
#include "TestWrapper.h"

// spa
#include "pkb.h"
#include "exceptions.h"
#include "simple/parser.h"
#include "design_extractor.h"
#include "pql/parser/parser.h"
#include "pql/eval/evaluator.h"

#include "util.h"


TestWrapper::TestWrapper() { }

TestWrapper::~TestWrapper() { }

void TestWrapper::parse(std::string filename)
{
    try
    {
        auto text = util::readEntireFile(filename.c_str());
        auto program = simple::parser::parseProgram(text);

        this->pkb = pkb::DesignExtractor(std::move(program)).run();
    }
    catch(const util::Exception& e)
    {
        util::logfmt("pkb", "exception caught during parsing or relation extraction: {}", e.what());
        this->pkb.reset(nullptr);
    }
}

void TestWrapper::evaluate(std::string query, std::list<std::string>& results)
{
    results.clear();
    if(!this->pkb)
    {
        util::logfmt("pql", "not evaluating query since PKB is invalid");
        return;
    }

    struct uwu
    {
        ~uwu()
        {
            util::Arena::global().clear();
        }
    } tmp;

    try
    {
        util::logfmt("pql:ast", "Starting to generate pql ast");

        auto query_ast = pql::parser::parsePQL(query);
        util::logfmt("pql:ast", "Generated AST: {}", query_ast->toString());

        auto eval = pql::eval::Evaluator(this->pkb.get(), std::move(query_ast));
        results = eval.evaluate();
    }
    catch(const util::Exception& e)
    {
        // Errors in query should be silently ignored.
        util::logfmt("pql", "exception caught during evaluating query: {}", e.what());
    }
}



// implementation code of WrapperFactory - do NOT modify the next 5 lines
AbstractWrapper* WrapperFactory::wrapper = 0;
AbstractWrapper* WrapperFactory::createWrapper()
{
    if(wrapper == 0)
        wrapper = new TestWrapper;

    return wrapper;
}

// Do not modify the following line
volatile bool AbstractWrapper::GlobalStop = false;
