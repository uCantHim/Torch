#include "PipelineDefinitionLanguage.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Scanner.h"
#include "Parser.h"
#include "AstPrinter.h"



constexpr int USAGE{ 64 };

void PipelineDefinitionLanguage::run(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: compile-pipeline [file]\n";
        exit(USAGE);
    }

    const bool result = compile(argv[1]);
    exit(result);
}

bool PipelineDefinitionLanguage::compile(const fs::path& filename)
{
    // Init
    errorReporter = std::make_unique<DefaultErrorReporter>(std::cout);

    std::ifstream file(filename);
    std::stringstream ss;
    ss << file.rdbuf();

    // Scan
    Scanner scanner(ss.str(), *errorReporter);
    auto tokens = scanner.scanTokens();

    // Parse
    Parser parser(std::move(tokens), *errorReporter);
    auto result = parser.parseTokens();
    if (errorReporter->hadError()) {
        return true;
    }

    ObjectDeclarationFieldValue rootField(std::move(result));

    // Print
    AstPrinter printer(rootField);
    printer.print();

    return errorReporter->hadError();
}
