#include <optional>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <trc/util/Exception.h>
#include <trc/util/ArgParse.h>
#include <trc/util/async/ThreadPool.h>
#include <nlohmann/json.hpp>
namespace nl = nlohmann;

#include <trc/shader_edit/Logger.h>
#include <trc/shader_edit/Compiler.h>
#include <trc/shader_edit/ConfigParserJson.h>
#include <trc/shader_edit/GenerateSpirv.h>
using namespace shader_edit;

constexpr const char* EXECUTABLE_NAME{ "shader_compiler" };

void printUsage()
{
    std::cout << "Usage: " << EXECUTABLE_NAME << " FILE [OPTIONS]\n"
        << "\n"
        << "Possible options:\n"
        << "\t--outDir DIRNAME\tWrite output files to directory DIRNAME\n"
        << "\t--genGlsl       \tGenerate GLSL files\n"
        << "\t--genSpirv      \tCompile generated GLSL code into SPIR-V and output it to files\n"
        << "\t--keep-existing \tDon't overwrite files that already exist in the filesystem\n"
        << "\t--verbose -v    \tEnable verbose output\n"
        << "\n";
}

auto compile(const fs::path& file, const trc::util::Args& args)
    -> std::optional<CompileResult>;
void writeToFile(const fs::path& outFile, const std::string& data, bool replaceExisting);

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

    info("Compilation completed");

    // Write compilation results to files
    const bool overwrite = !trc::util::hasNamedArg(args, "--keep-existing");
    const bool genGlsl = trc::util::hasNamedArg(args, "--genGlsl");
    const bool genSpirv = trc::util::hasNamedArg(args, "--genSpv");

    if (!genGlsl && !genSpirv)
    {
        std::cout << "Neither --genGlsl nor --genSpv specified, no output will be produced.\n";
        return 0;
    }

    { // scope for thread pool synchronization
    trc::async::ThreadPool tp{ std::thread::hardware_concurrency() };
    for (auto& shader : result.value().shaderFiles)
    {
        if (genGlsl) {
            writeToFile(shader.filePath, shader.code, overwrite);
        }
        if (genSpirv)
        {
            tp.async([&, shader=std::move(shader)] {
                auto spv = generateSpirv(shader);
                if (spv.GetNumErrors() > 0)
                {
                    error("Unable to compile " + shader.filePath.string() + " to SPIR-V: "
                          + spv.GetErrorMessage());
                    return;
                }

                std::vector<uint32_t> spvVec{ spv.begin(), spv.end() };
                std::string spvData;
                spvData.resize(spvVec.size() * sizeof(uint32_t));
                memcpy(spvData.data(), spvVec.data(), spvVec.size() * sizeof(uint32_t));

                fs::path spvPath{ shader.filePath.string() + ".spv" };
                writeToFile(spvPath, spvData, overwrite);
            });
        }
    }
    } // scope

    std::cout << "\nCompilation successful.\n";
}

auto compile(const fs::path& filePath, const trc::util::Args& args)
    -> std::optional<CompileResult>
{
    try {
        std::cout << "Reading configuration from " << filePath << "\n";

        std::ifstream file{ filePath };
        auto compileConfig = CompileConfiguration::fromJson(file);
        if (trc::util::hasNamedArg(args, "--outDir")) {
            compileConfig.meta.outDir = trc::util::getNamedArg(args, "--outDir");
        }

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

void writeToFile(
    const fs::path& outFile,
    const std::string& data,
    const bool replaceExisting)
{
    if (!replaceExisting && fs::is_regular_file(outFile))
    {
        info("File " + outFile.string() + " already exists in the filesystem. Skipping.");
        return;
    }

    fs::create_directories(outFile.parent_path()); // Ensure that all parent dirs exist
    std::ofstream out{ outFile };
    if (!out.is_open())
    {
        warn("Unable to open file " + outFile.string() + " for writing. Skipping.");
        return;
    }

    out << data;
    info("Generated file " + outFile.string());
}
