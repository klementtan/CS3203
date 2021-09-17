#ifndef TESTWRAPPER_H
#define TESTWRAPPER_H

#include <list>
#include <string>
#include <memory>

// include your other headers here
#include "AbstractWrapper.h"

namespace pkb
{
    struct ProgramKB;
}

namespace simple::ast
{
    struct Program;
}

class TestWrapper : public AbstractWrapper
{
public:
    TestWrapper();
    ~TestWrapper();

    virtual void parse(std::string filename);
    virtual void evaluate(std::string query, std::list<std::string>& results);

private:
    std::unique_ptr<pkb::ProgramKB> pkb {};
};

#endif
