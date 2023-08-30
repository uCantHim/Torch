#include "Importer.h"

#include <fstream>
#include <sstream>

#include "ErrorReporter.h"
#include "Scanner.h"
#include "Parser.h"



Importer::Importer(std::vector<fs::path> includePaths, ErrorReporter& errorReporter)
    :
    includePaths(std::move(includePaths)),
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
    const auto importedFile = findFile(stmt.importString);
    if (!importedFile)
    {
        errorReporter->error(Error{
            .location=stmt.token.location,
            .message="Import " + stmt.importString + " not found."
        });
    }

    std::ifstream file(*importedFile);
    if (!file.is_open())
    {
        errorReporter->error(Error{
            .location=stmt.token.location,
            .message="Unable to open file " + stmt.importString + "."
        });
        return;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    Scanner scanner(ss.str(), *errorReporter);
    auto tokens = scanner.scanTokens();

    Parser parser(std::move(tokens), *errorReporter);
    auto stmts = parser.parseTokens();

    // Recursively resolve import statements in imported source
    Importer importer(includePaths, *errorReporter);
    auto imports = importer.parseImports(stmts);

    // Append to result
    std::move(stmts.begin(), stmts.end(), std::back_inserter(results));
    std::move(imports.begin(), imports.end(), std::back_inserter(results));
}

auto Importer::findFile(const std::string& importString) -> std::optional<fs::path>
{
    for (const auto& path : includePaths)
    {
        fs::path filePath = path / importString;
        if (fs::is_regular_file(filePath)) {
            return filePath;
        }
    }

    return std::nullopt;
}
