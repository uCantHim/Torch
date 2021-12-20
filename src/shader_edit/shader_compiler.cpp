#include <optional>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <trc/util/Exception.h>
#include <trc/util/ArgParse.h>
#include <nlohmann/json.hpp>
namespace nl = nlohmann;

#include "Logger.h"
#include "Compiler.h"
#include "ConfigParserJson.h"
using namespace shader_edit;

constexpr const char* EXECUTABLE_NAME{ "compiler" };

void printUsage()
{
    std::cout << "Usage: " << EXECUTABLE_NAME << " FILE [OPTIONS]\n"
        << "\n"
        << "Possible options:\n"
        << "\t--outDir DIRNAME\tWrite output files to directory DIRNAME\n"
        << "\t--verbose -v \tVerbose output\n"
        << "\n";
}

auto compile(const fs::path& file, const trc::util::Args& args)
    -> std::optional<CompileResult>;
void writeToFile(const CompiledShaderFile& data);

int main(int argc, const char* argv[])
{
    auto args = trc::util::parseArgs(argc, argv);
    const bool verbose = trc::util::hasNamedArg(args, "--verbose")
                         || trc::util::hasNamedArg(args, "-v");
    verboseLogging = verbose;

    if (args.size() < 2)
    {
        printUsage();
        exit(1);
    }

    fs::path inFile{ args[1] };
    if (!fs::is_regular_file(inFile))
    {
        std::cout << "File " << inFile << " is not a regular file. Exiting.\n";
        exit(1);
    }

    // Compile
    auto result = compile(inFile, args);
    if (!result.has_value())
    {
        std::cout << "Compilation failed. Exiting.\n";
        exit(1);
    }

    // Write compilation result to output file
    auto [name, outDir] = trc::util::getNamedArgOr(args, "--outDir", ".");

    info("Compilation completed\n");
    for (const auto& shader : result.value().shaderFiles)
    {
        info("Generated shader " + shader.filePath.string());
        writeToFile(shader);
    }

    std::cout << "\nCompilation successful.\n";
}

auto compile(const fs::path& filePath, const trc::util::Args& args)
    -> std::optional<CompileResult>
{
    try {
        std::cout << "Parsing file " << filePath << "...\n";
        std::ifstream file{ filePath };
        auto compileConfig = CompileConfiguration::fromJson(file);

        std::cout << "Compiling file " << filePath << "...\n";
        Compiler compiler;
        return compiler.compile(std::move(compileConfig));
    }
    catch (const nl::json::parse_error& err) {
        std::cout << "JSON syntax error: " << err.what() << "\n";
        return std::nullopt;
    }
    catch (const ParseError& err) {
        std::cout << "Parse error: " << err.what() << "\n";
        return std::nullopt;
    }
    catch (const CompileError& err) {
        std::cout << "Compile error: " << err.what() << "\n";
        return std::nullopt;
    }
}

void writeToFile(const CompiledShaderFile& data)
{
    fs::path newFilename{ data.filePath.filename().string() + ".out" };
    fs::path outFile{ data.filePath.parent_path() / newFilename };
    if (fs::is_regular_file(outFile))
    {
        info("File " + outFile.string() + " already exists in the filesystem. Skipping.\n");
        return;
    }

    fs::create_directories(outFile.parent_path()); // Ensure that all parent dirs exist
    std::ofstream out{ outFile };
    if (!out.is_open())
    {
        info("Unable to open file " + outFile.string() + " for writing. Skipping.\n");
    }

    out << data.code;
}
