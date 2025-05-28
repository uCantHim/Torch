#include <iostream>
#include <filesystem>
#include <fstream>

#include <argparse/argparse.hpp>
#include <shader_tools/ShaderDocument.h>
#include <trc/assets/MaterialRegistry.h>
#include <trc/assets/SimpleMaterial.h>
#include <trc_util/Timer.h>

#include <cloth/cloth.h>
#include <cloth/parser.h>
#include <cloth/torch_impl.h>

namespace fs = std::filesystem;

auto compileToMaterial(std::istream& is) -> std::expected<trc::MaterialData, cloth::CompileError>
{
    cloth::TorchImpl impl;

    trc::Timer timer;
    auto res = cloth::compileShader(is, impl.makeCapabilityConfig(), impl.makeOutputConfig());
    std::cout << "Cloth shader processed in " << timer.reset() << "ms\n";

    if (res) {
        return trc::makeMaterial({ .fragmentModule=res->shaderModule, .transparent=false });
    }
    return std::unexpected(res.error());
}

void outputErrors(cloth::CompileError& doc, std::optional<std::string> filePath)
{
    std::cout << cloth::printErrors(doc, filePath);
    exit(1);
}

constexpr int kInvalidUsageExitcode{ 64 };

void configureArgumentParser(argparse::ArgumentParser& prog)
{
    prog.add_description("Compile Cloth shader code to Torch materials.");

    prog.add_argument("file")
        .help("A Cloth shader file. Omit to read from STDIN.");
    prog.add_argument("--output", "-o")
        .help("Path to an output file. Omit to construct a default file name."
              " Specify '-' to write output to STDOUT.");
}

int main(int argc, const char** argv)
{
    argparse::ArgumentParser program;
    configureArgumentParser(program);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << program;
        exit(kInvalidUsageExitcode);
    }

    auto outputFilePath = [&program] -> std::optional<fs::path> {
        if (auto outFile = program.present("output"))
        {
            if (*outFile == "-") {
                return std::nullopt;
            }
            return *outFile;
        }
        if (auto fileName = program.present("file")) {
            return fs::path{*fileName}.replace_extension(".mat.ta");
        }
        return std::nullopt;
    }();

    auto writeOutput = [&outputFilePath](const trc::MaterialData& data)
    {
        if (outputFilePath)
        {
            std::ofstream outFile{ *outputFilePath };
            trc::serializeAsset(data, outFile);
        }
        else {
            trc::serializeAsset(data, std::cout);
        }
    };

    if (auto fileName = program.present("file"))
    {
        std::ifstream file{ *fileName };
        auto res = compileToMaterial(file);
        if (res) {
            writeOutput(*res);
        }
        else {
            outputErrors(res.error(), fileName);
        }
    }
    else {
        auto res = compileToMaterial(std::cin);
        if (res) {
            writeOutput(*res);
        }
        else {
            outputErrors(res.error(), "STDIN");
        }
    }

    return 0;
}
