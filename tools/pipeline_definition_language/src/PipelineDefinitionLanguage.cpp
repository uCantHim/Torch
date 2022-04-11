#include "PipelineDefinitionLanguage.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Scanner.h"



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

    // Compile
    Scanner scanner(ss.str(), *errorReporter);
    auto tokens = scanner.scanTokens();

    for (const Token& token : tokens)
    {
        std::cout << "[line " << token.location.line << "]: "
            << to_string(token.type) << " \"" << token.lexeme << "\"" << "\n";
    }

    return errorReporter->hadError();
}
