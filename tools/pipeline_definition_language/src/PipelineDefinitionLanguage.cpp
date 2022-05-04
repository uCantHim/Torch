#include "PipelineDefinitionLanguage.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include <argparse/argparse.hpp>

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
    argparse::ArgumentParser program("compiler", "0.1");
    program.add_argument("file")
        .help("Input file");
    program.add_argument("-o", "--output")
        .help("Output directory. Generated files are stored here.");
    program.add_argument("-i", "--input")
        .help("Input directory. Shader file paths are interpreted relative to this path.");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << program;
        exit(USAGE);
    }

    const fs::path filename = program.get<std::string>("file");
    if (!fs::is_regular_file(filename))
    {
        std::cerr << filename << " is not a regular file. Exiting.";
        exit(USAGE);
    }

    if (auto val = program.present("-o"))
    {
        outputDir = *val;
        outputFileName = filename.filename();
        fs::create_directories(outputDir);
    }

    if (auto val = program.present("-i"))
    {
        inputDir = *val;
        if (!fs::is_directory(inputDir))
        {
            std::cerr << inputDir << " is not a directory. Exiting.";
            exit(USAGE);
        }
    }

    // Init
    errorReporter = std::make_unique<DefaultErrorReporter>(std::cout);
    try {
        const auto result = compile(filename);
        if (!result) exit(1);

        writeOutput(result.value());
    }
    catch (const InternalLogicError& err) {
        std::cout << "\n[INTERNAL COMPILER ERROR]: " << err.message << "\n";
        exit(1);
    }
    catch (const std::runtime_error& err) {
        std::cout << "An unexpected error occured: " << err.what() << "\n" << "Exiting.";
        exit(1);
    }
}

auto PipelineDefinitionLanguage::compile(const fs::path& filename) -> std::optional<CompileResult>
{
    std::ifstream file(filename);
    std::stringstream ss;
    ss << file.rdbuf();

    // Scan
    Scanner scanner(ss.str(), *errorReporter);
    auto tokens = scanner.scanTokens();
    if (errorReporter->hadError()) return std::nullopt;

    // Parse
    Parser parser(std::move(tokens), *errorReporter);
    auto parseResult = parser.parseTokens();

    // Load standard library
    auto stdlib = loadStdlib(*errorReporter);
    if (errorReporter->hadError()) return std::nullopt;
    std::move(stdlib.begin(), stdlib.end(), std::back_inserter(parseResult));

    // Load additional types defined in the input file
    auto typeConfig = makeDefaultTypeConfig();
    TypeParser typeParser(typeConfig, *errorReporter);
    typeParser.parse(parseResult);

    // Check types
    TypeChecker typeChecker(std::move(typeConfig), *errorReporter);
    typeChecker.check(parseResult);

    // Don't try to compile if errors have occured thus far
    if (errorReporter->hadError()) return std::nullopt;

    // Compile
    Compiler compiler(std::move(parseResult), *errorReporter);
    auto compileResult = compiler.compile();

    // Certainly don't output anything if errors have occured
    if (errorReporter->hadError()) return std::nullopt;
    return compileResult;
}


void PipelineDefinitionLanguage::writeOutput(const CompileResult& result)
{
    TorchCppWriter writer(*errorReporter, {
        .shaderInputDir=inputDir,
        .shaderOutputDir=outputDir
    });

    fs::path outFileName = outputDir / outputFileName;
    if (generateHeader)
    {
        const fs::path headerName = outFileName.replace_extension(".h");
        std::ofstream header(headerName);
        std::ofstream source(outFileName.replace_extension(".cpp"));
        source << "#include " << headerName << "\n\n";
        writer.write(result, header, source);
    }
    else {
        std::ofstream file(outFileName.replace_extension(".h"));
        writer.write(result, file);
    }

    // Copy helper files
    std::ifstream inFile(FLAG_COMBINATION_HEADER);
    std::ofstream outFile(outputDir / "FlagCombination.h");
    outFile << inFile.rdbuf();
}
