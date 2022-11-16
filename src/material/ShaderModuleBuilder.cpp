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

}
