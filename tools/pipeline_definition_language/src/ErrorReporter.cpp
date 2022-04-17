#include "ErrorReporter.h"



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
    *os << "[line " << error.location.line << ":" << error.location.start << "] "
        << error.message << "\n";
}
