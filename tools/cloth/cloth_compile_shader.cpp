#include <iostream>
#include <fstream>

#include <argparse/argparse.hpp>
#include <shader_tools/ShaderDocument.h>
#include <trc/assets/MaterialRegistry.h>
#include <trc/assets/SimpleMaterial.h>
#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc_util/Timer.h>

#include <cloth/cloth.h>
#include <cloth/parser.h>

struct DeferredFragmentShaderImpl : cloth::ShaderOutputImpl
{
    DeferredFragmentShaderImpl()
    {
        defineParameter("color", glm::vec4{});
        defineParameter("normal", glm::vec3{});
        defineParameter("specularFactor", float{});
        defineParameter("emissive", bool{});
        defineParameter("roughness", float{});
        defineParameter("metallicness", float{});
    }

    void setParameter(const std::string& outputName,
                      trc::shader::code::Value value) override
    {
        using Param = trc::FragmentModule::Parameter;
        static const std::unordered_map<std::string, Param> map{
            { "color", Param::eColor },
            { "normal", Param::eNormal },
            { "specularFactor", Param::eSpecularFactor },
            { "metallicness", Param::eMetallicness },
            { "roughness", Param::eRoughness },
            { "emissive", Param::eEmissive },
        };

        if (map.contains(outputName)) {
            frag.setParameter(map.at(outputName), value);
        }
    }

    auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
        -> trc::shader::ShaderOutputInterface override
    {
        const bool transparent = false;
        return frag.buildOutputs(builder, transparent);
    }

    trc::FragmentModule frag;
};

auto compileToMaterial(std::istream& is) -> std::expected<trc::MaterialData, cloth::CompileError>
{
    auto capabilityConfig = trc::makeFragmentCapabilityConfig();
    auto outputConfig = DeferredFragmentShaderImpl{};

    trc::Timer timer;
    auto res = cloth::compileShader(is, capabilityConfig, outputConfig);
    std::cout << "Cloth shader processed in " << timer.reset() << "ms\n";

    if (res) {
        return trc::makeMaterial({ .fragmentModule=res->shaderModule, .transparent=false });
    }
    return std::unexpected(res.error());
}

void outputErrors(cloth::CompileError& doc, std::optional<std::string> filePath)
{
    auto indent = [](size_t n, char c = ' ') { return std::string(n, c); };

    std::cout << "Compile error.\n";
    for (const auto& err : doc.errors)
    {
        const auto& lines = doc.initialDocumentLines;
        const auto loc = err.location;

        // Error message
        if (filePath) {
            std::cout << *filePath << ":";
        }
        std::cout << loc.line << ":" << loc.firstChar << ": error: " << err.message << "\n";
        // Code line
        std::cout << "  " << loc.line << " | " << lines.at(loc.line) << "\n";
        // Positional indiator line
        std::cout << "  " << indent(std::to_string(loc.line).size())
                  << " | " << indent(loc.firstChar)
                  << "^" << indent(loc.endChar - loc.firstChar - 1, '~') << "\n";
    }
}

constexpr int kInvalidUsageExitcode{ 64 };

void configureArgumentParser(argparse::ArgumentParser& prog)
{
    prog.add_description("Compile Cloth shader code to GLSL or SPIR-V.");

    prog.add_argument("file")
        .help("A Cloth shader file. Omit to read from STDIN.");
    prog.add_argument("--output", "-o")
        .help("Path to an output file. Omit to construct a default file name."
              " Specify '-' to write output to STDOUT.");

    auto& outputType = prog.add_mutually_exclusive_group();
    outputType.add_argument("--glsl")
              .implicit_value(true)
              .default_value(true)
              .help("Output GLSL code.");
    outputType.add_argument("--spirv")
              .implicit_value(true)
              .default_value(false)
              .help("Output SPIR-V code.");
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

    if (auto fileName = program.present("file"))
    {
        std::ifstream file{ *fileName };
        auto res = compileToMaterial(file);
        if (res)
        {
            std::ofstream outFile{ *fileName + ".out" };
            trc::AssetSerializerTraits<trc::Material>::serialize(*res, outFile);
        }
        else {
            outputErrors(res.error(), fileName);
        }
    }
    else {
        auto res = compileToMaterial(std::cin);
        if (res) {
            std::ofstream outFile{ program.get("output") };
            trc::MaterialData::serialize(*res, outFile);
        }
        else {
            outputErrors(res.error(), "STDIN");
        }
    }

    return 0;
}
