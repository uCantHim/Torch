#include "ErrorReporter.h"

#include <iostream>



void ErrorReporter::error(const Error& error)
{
    ++numErrors;
    reportError(error);
}

bool ErrorReporter::hadError() const
{
    return numErrors > 0;
}



DefaultErrorReporter::DefaultErrorReporter(std::ostream& os)
    :
    os(&os)
{
}

void DefaultErrorReporter::reportError(const Error& error)
{
    *os << "Error in line " << error.line << ": " << error.message << "\n";
}
