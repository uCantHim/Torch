#include "PipelineDefinitionLanguage.h"

#include <vector>
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



auto loadStdlib(ErrorReporter& errorReporter) -> std::vector<Stmt>
{
    std::vector<Stmt> statements;

    for (auto& entry : fs::directory_iterator(STDLIB_DIR))
    {
        if (!entry.is_regular_file()) continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open standard library file " + entry.path().string());
        }
        std::stringstream ss;
        ss << file.rdbuf();

        // Scan
        Scanner scanner(ss.str(), errorReporter);
        auto tokens = scanner.scanTokens();

        // Parse
        Parser parser(std::move(tokens), errorReporter);
        auto parseResult = parser.parseTokens();

        statements.insert(statements.begin(), parseResult.begin(), parseResult.end());
    }

    return statements;
}



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
    catch (const std::runtime_error& err) {
        std::cout << "An error occured: " << err.what() << "\n" << "Exiting.";
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

    // Load standard library
    auto stdlib = loadStdlib(*errorReporter);
    if (errorReporter->hadError()) {
        return true;
    }
    std::move(stdlib.begin(), stdlib.end(), std::back_inserter(parseResult));

    // Load additional types defined in the input file
    auto typeConfig = makeDefaultTypeConfig();
    TypeParser typeParser(typeConfig, *errorReporter);
    typeParser.parse(parseResult);

    // Check types
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
