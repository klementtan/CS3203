// TestWrapper.cpp

#include "TestWrapper.h"

// spa
#include "util.h"
#include "simple_parser.h"
#include "pql/parser.h"
#include "pql/exception.h"


TestWrapper::TestWrapper() { }

void TestWrapper::parse(std::string filename)
{
    auto text = util::readEntireFile(filename.c_str());
    auto program = simple::parser::parseProgram(text);
}


void TestWrapper::evaluate(std::string query, std::list<std::string>& results)
{
    try
    {
        util::log("pql:ast", "Starting to generate pql ast");
        pql::ast::Query* query_ast = pql::parser::parsePQL(query);
        util::log("pql:ast", "Generated AST: {}", query_ast->toString());
    } catch(const pql::exception::PqlException& e)
    {
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
