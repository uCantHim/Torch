#include "trc/material/ShaderModuleBuilder.h"



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

auto ShaderModuleBuilder::getCapabilityConfig() const -> const ShaderCapabilityConfig&
{
    return config;
}

auto ShaderModuleBuilder::compileResourceDecls() const -> ShaderResources
{
    return resources.compile();
}

auto ShaderModuleBuilder::getPrimaryBlock() const -> Block
{
    return getFunction("main").value()->getBlock();
}

}
