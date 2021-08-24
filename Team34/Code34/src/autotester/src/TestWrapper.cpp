// TestWrapper.cpp

#include "TestWrapper.h"

// spa
#include "frontend.h"


TestWrapper::TestWrapper() { }

void TestWrapper::parse(std::string filename)
{
    auto text = frontend::readEntireFile(filename.c_str());
    auto program = frontend::parseProgram(text);
}


void TestWrapper::evaluate(std::string query, std::list<std::string>& results) { }






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
