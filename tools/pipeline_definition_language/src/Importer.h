#pragma once

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
    Importer(fs::path filePath, ErrorReporter& errorReporter);

    auto parseImports(const std::vector<Stmt>& statements) -> std::vector<Stmt>;

    void operator()(const ImportStmt& stmt);
    void operator()(const TypeDef&) {}
    void operator()(const FieldDefinition&) {}

private:
    const fs::path filePath;
    ErrorReporter* errorReporter;

    std::vector<Stmt> results;
};
