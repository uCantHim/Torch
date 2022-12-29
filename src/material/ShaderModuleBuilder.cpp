#include "trc/material/ShaderModuleBuilder.h"

#include <shader_tools/ShaderDocument.h>

#include "trc/material/ShaderCodeCompiler.h"



namespace trc
{

ShaderFunction::ShaderFunction(const std::string& name, FunctionType type)
    :
    name(name),
    signature(std::move(type))
{
}

auto ShaderFunction::getName() const -> const std::string&
{
    return name;
}

auto ShaderFunction::getType() const -> const FunctionType&
{
    return signature;
}



ShaderModuleBuilder::ShaderModuleBuilder(ShaderCapabilityConfig conf)
    :
    config(std::move(conf)),
    resources(config, *this)
{
    // I can declare the main function like this because function declarations
    // are always compiled in the reversed order in which they were created.
    // Main is always the first function that is declared.
    auto main = makeFunction("main", FunctionType{ {}, std::nullopt });
    startBlock(main);
}

auto ShaderModuleBuilder::makeCapabilityAccess(Capability capability) -> Value
{
    return resources.queryCapability(capability);
}

auto ShaderModuleBuilder::makeTextureSample(TextureReference tex, Value uvs) -> Value
{
    return makeExternalCall("texture", { resources.queryTexture(tex), uvs });
}

void ShaderModuleBuilder::includeCode(
    util::Pathlet path,
    std::unordered_map<std::string, Capability> varReplacements)
{
    auto it = std::ranges::find_if(includedFiles,
                                   [&path](auto& a){ return a.first == path; });

    // Only insert the pair if `path` does not yet exist in the list
    if (it == includedFiles.end())
    {
        includedFiles.push_back({ path, {} });
        auto& [_, map] = includedFiles.back();
        for (auto& [name, capability] : varReplacements) {
            map.try_emplace(std::move(name), makeCapabilityAccess(capability));
        }
    }
}

void ShaderModuleBuilder::enableEarlyFragmentTest()
{
    shaderSettings.earlyFragmentTests = true;
}

auto ShaderModuleBuilder::getCapabilityConfig() const -> const ShaderCapabilityConfig&
{
    return config;
}

auto ShaderModuleBuilder::getSettings() const -> const Settings&
{
    return shaderSettings;
}

auto ShaderModuleBuilder::compileResourceDecls() const -> ShaderResources
{
    return resources.compile();
}

auto ShaderModuleBuilder::compileIncludedCode(shaderc::CompileOptions::IncluderInterface& includer)
    -> std::string
{
    std::string result;
    for (const auto& [path, vars] : includedFiles)
    {
        auto str = path.string();
        auto include = includer.GetInclude(
            str.c_str(), shaderc_include_type_standard,
            "shader_module_dummy_include_name", 1);

        if (include->source_name_length == 0) {
            throw std::runtime_error("[In ShaderModuleBuilder::compileIncludedCode]: "
                                     + std::string(include->content));
        }

        std::ifstream file(include->source_name);
        includer.ReleaseInclude(include);

        shader_edit::ShaderDocument doc(file);
        for (const auto& [name, value] : vars)
        {
            auto [id, code] = ShaderValueCompiler{ true }.compile(value);
            doc.set(name, id);
        }

        result += doc.compile();
    }

    return result;
}

auto ShaderModuleBuilder::getPrimaryBlock() const -> Block
{
    return getFunction("main").value()->getBlock();
}

}
