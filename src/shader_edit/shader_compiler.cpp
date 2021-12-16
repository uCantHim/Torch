#include <optional>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <trc/util/Exception.h>
#include <trc/util/ArgParse.h>
#include <nlohmann/json.hpp>
namespace nl = nlohmann;

#include "Compiler.h"
#include "ConfigParserJson.h"

constexpr const char* EXECUTABLE_NAME{ "compiler" };
constexpr const char* OUTPUT_FILE_OPTION_NAME{ "-o" };

void printUsage()
{
    std::cout << "Usage: " << EXECUTABLE_NAME << " FILE [OPTIONS]\n"
        << "\n"
        << "Possible options:\n"
        << "\t-o FILENAME\tWrite output to file FILENAME\n"
        << "\n";
}

auto compile(const fs::path& file, const trc::util::Args& args)
    -> std::optional<shader_edit::CompileResult>;
void writeToFile(const shader_edit::CompiledShaderFile& data);

int main(int argc, const char* argv[])
{
    auto args = trc::util::parseArgs(argc, argv);

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
    auto [name, value] = trc::util::getNamedArgOr(
        args,
        OUTPUT_FILE_OPTION_NAME,
        inFile.string() + ".out"
    );

    std::cout << "\nCompilation completed\n";
    for (const auto& shader : result.value().shaderFiles)
    {
        std::cout << "--- Generated shader " << shader.filePath << "\n";
        writeToFile(shader);
    }

    std::cout << "\nCompilation successful.\n";
}

auto compile(const fs::path& filePath, const trc::util::Args& args)
    -> std::optional<shader_edit::CompileResult>
{
    try {
        std::cout << "Parsing file " << filePath << "...\n";
        std::ifstream file{ filePath };
        auto compileConfig = shader_edit::CompileConfiguration::fromJson(file);

        std::cout << "Compiling file " << filePath << "...\n";
        shader_edit::Compiler compiler;
        return compiler.compile(std::move(compileConfig));
    }
    catch (const nl::json::parse_error& err) {
        std::cout << "JSON syntax error: " << err.what() << "\n";
        return std::nullopt;
    }
    catch (const shader_edit::ParseError& err) {
        std::cout << "Parse error: " << err.what() << "\n";
        return std::nullopt;
    }
    catch (const shader_edit::CompileError& err) {
        std::cout << "Compile error: " << err.what() << "\n";
        return std::nullopt;
    }
}

void writeToFile(const shader_edit::CompiledShaderFile& data)
{
    fs::path newFilename{ data.filePath.filename().string() + ".out" };
    fs::path outFile{ data.filePath.parent_path() / newFilename };
    if (fs::is_regular_file(outFile))
    {
        std::cout << "File " << outFile << " already exists. Skipping.\n";
        return;
    }

    std::ofstream out{ outFile };
    if (!out.is_open())
    {
        std::cout << "Unable to open file " << outFile << " for writing. Skipping.\n";
    }
}
