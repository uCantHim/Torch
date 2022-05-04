#pragma once

#include <ostream>

#include "CompileResult.h"

class Writer
{
public:
    virtual void write(const CompileResult& result, std::ostream& os) = 0;
    virtual void write(const CompileResult& result, std::ostream& header, std::ostream& src) = 0;
};
