// TestWrapper.cpp

#include "TestWrapper.h"

// spa
#include "util.h"
#include "simple/parser.h"
#include "pql/parser/parser.h"
#include "pql/exception.h"
#include "pkb.h"
#include "pql/eval/evaluator.h"


TestWrapper::TestWrapper() { }

void TestWrapper::parse(std::string filename)
{
    auto text = util::readEntireFile(filename.c_str());
    auto program = simple::parser::parseProgram(text).unwrap();
    this->pkb = pkb::processProgram(program).unwrap();
}


void TestWrapper::evaluate(std::string query, std::list<std::string>& results)
{
    try
    {
        util::log("pql:ast", "Starting to generate pql ast");
        pql::ast::Query* query_ast = pql::parser::parsePQL(query);
        util::log("pql:ast", "Generated AST: {}", query_ast->toString());
        pql::eval::Evaluator* eval = new pql::eval::Evaluator(this->pkb, query_ast);
        results = eval->evaluate();
    } catch(const pql::exception::PqlException& e)
    {
        // Errores in query should be silently ignored.
        util::log("pql", "PqlException caught during evaluating query. PqlException: {}", e.what());
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
