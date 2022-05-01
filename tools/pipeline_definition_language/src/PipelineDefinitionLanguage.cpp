#include "PipelineDefinitionLanguage.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Exceptions.h"
#include "Scanner.h"
#include "Parser.h"
#include "TypeParser.h"
#include "TypeChecker.h"
#include "Compiler.h"
#include "TorchCppWriter.h"



constexpr int USAGE{ 64 };

void PipelineDefinitionLanguage::run(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: compile-pipeline [file]\n";
        exit(USAGE);
    }

    try {
        const bool result = compile(argv[1]);
        exit(result);
    }
    catch (const InternalLogicError& err) {
        std::cout << "\n[INTERNAL COMPILER ERROR]: " << err.message << "\n";
        exit(1);
    }
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
    auto parseResult = parser.parseTokens();

    // Check types
    auto typeConfig = makeDefaultTypeConfig();
    TypeParser typeParser(typeConfig, *errorReporter);
    typeParser.parse(parseResult);

    TypeChecker typeChecker(std::move(typeConfig), *errorReporter);
    typeChecker.check(parseResult);

    // Don't try to compile if errors have occured thus far
    if (errorReporter->hadError()) {
        return true;
    }

    // Compile
    Compiler compiler(std::move(parseResult), *errorReporter);
    auto compileResult = compiler.compile();

    // Certainly don't output anything if errors have occured
    if (errorReporter->hadError()) {
        return true;
    }

    // Output
    TorchCppWriter writer(*errorReporter);
    writer.write(compileResult, std::cout);

    return errorReporter->hadError();
}
