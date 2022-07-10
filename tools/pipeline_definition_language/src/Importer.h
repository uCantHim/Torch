#pragma once

#include <optional>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include "SyntaxElements.h"

class ErrorReporter;

class Importer
{
public:
    /**
     * @param fs::path filePath Path of the compiled file
     */
    Importer(std::vector<fs::path> includePaths, ErrorReporter& errorReporter);

    auto parseImports(const std::vector<Stmt>& statements) -> std::vector<Stmt>;

    void operator()(const ImportStmt& stmt);
    void operator()(const TypeDef&) {}
    void operator()(const FieldDefinition&) {}

private:
    auto findFile(const std::string& importString) -> std::optional<fs::path>;

    const std::vector<fs::path> includePaths;
    ErrorReporter* errorReporter;

    std::vector<Stmt> results;
};
