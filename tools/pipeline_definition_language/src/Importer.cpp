#include "Importer.h"

#include <fstream>

#include "ErrorReporter.h"
#include "Scanner.h"
#include "Parser.h"



Importer::Importer(fs::path filePath, ErrorReporter& errorReporter)
    :
    filePath(std::move(filePath)),
    errorReporter(&errorReporter)
{
}

auto Importer::parseImports(const std::vector<Stmt>& statements) -> std::vector<Stmt>
{
    for (const auto& stmt : statements)
    {
        std::visit(*this, stmt);
    }

    return std::move(results);
}

void Importer::operator()(const ImportStmt& stmt)
{
    const auto importedFile = filePath.parent_path() / stmt.importString;
    std::ifstream file(importedFile);
    if (!file.is_open())
    {
        errorReporter->error(Error{
            .location=stmt.token.location,
            .message="Unable to open file " + stmt.importString + "."
        });
    }

    std::stringstream ss;
    ss << file.rdbuf();

    Scanner scanner(ss.str(), *errorReporter);
    auto tokens = scanner.scanTokens();

    Parser parser(std::move(tokens), *errorReporter);
    auto stmts = parser.parseTokens();

    // Recursively resolve import statements in imported source
    Importer importer(importedFile, *errorReporter);
    auto imports = importer.parseImports(stmts);

    // Append to result
    std::move(stmts.begin(), stmts.end(), std::back_inserter(results));
    std::move(imports.begin(), imports.end(), std::back_inserter(results));
}
