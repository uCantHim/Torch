#include "Compiler.h"

#include "FlagTable.h"
#include "VariantResolver.h"



auto Compiler::compile(const std::vector<Stmt>& statements) -> CompileResult
{
    CompileResult result;

    FlagTable flagTable = FlagTypeCollector{}.collect(statements);
    VariantResolver resolver(flagTable);

    result.flagTypes = flagTable.getAllFlags();

    return result;
}
