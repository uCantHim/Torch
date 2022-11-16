#include "trc/material/ShaderCapabilityConfig.h"

#include <stdexcept>



namespace trc
{

auto ShaderCapabilityConfig::getCodeBuilder() -> ShaderCodeBuilder&
{
    return *codeBuilder;
}

void ShaderCapabilityConfig::addGlobalShaderExtension(std::string extensionName)
{
    globalExtensions.emplace(std::move(extensionName));
}

void ShaderCapabilityConfig::addGlobalShaderInclude(util::Pathlet includePath)
{
    globalIncludes.emplace(std::move(includePath));
}

auto ShaderCapabilityConfig::getGlobalShaderExtensions() const
    -> const std::unordered_set<std::string>&
{
    return globalExtensions;
}

auto ShaderCapabilityConfig::getGlobalShaderIncludes() const
    -> const std::unordered_set<util::Pathlet>&
{
    return globalIncludes;
}

auto ShaderCapabilityConfig::addResource(Resource shaderResource) -> ResourceID
{
    const ResourceID id{ static_cast<ui32>(resources.size()) };
    const std::string name = "_access_resource_" + std::to_string(id);

    resources.emplace_back(new ResourceData{ std::move(shaderResource), name, {}, {}, {} });
    resourceAccessors.try_emplace(id, codeBuilder->makeExternalIdentifier(name));

    return id;
}

void ShaderCapabilityConfig::addShaderExtension(ResourceID resource, std::string extensionName)
{
    resources.at(resource)->extensions.emplace(std::move(extensionName));
}

void ShaderCapabilityConfig::addShaderInclude(ResourceID resource, util::Pathlet includePath)
{
    resources.at(resource)->includeFiles.emplace(std::move(includePath));
}

void ShaderCapabilityConfig::addMacro(
    ResourceID resource,
    std::string name,
    std::optional<std::string> value)
{
    resources.at(resource)->macroDefinitions.try_emplace(std::move(name), std::move(value));
}

auto ShaderCapabilityConfig::accessResource(ResourceID resource) const -> code::Value
{
    return resourceAccessors.at(resource);
}

auto ShaderCapabilityConfig::getResource(ResourceID resource) const -> const ResourceData&
{
    assert(resource < resources.size());
    return *resources.at(resource);
}

void ShaderCapabilityConfig::linkCapability(
    Capability capability,
    ResourceID resource,
    BasicType type)
{
    return linkCapability(capability, accessResource(resource), type, { resource });
}

void ShaderCapabilityConfig::linkCapability(
    Capability capability,
    code::Value value,
    BasicType type,
    std::vector<ResourceID> resources)
{
    auto [_, success] = capabilityAccessors.try_emplace(capability, value);
    if (!success) {
        throw std::invalid_argument("[In ShaderCapabilityConfig::linkCapability]: The capability"
                                    " is already linked to a resource!");
    }

    capabilityTypes.try_emplace(capability, type);
    requiredResources.try_emplace(capability, resources.begin(), resources.end());
}

bool ShaderCapabilityConfig::hasCapability(Capability capability) const
{
    return capabilityAccessors.contains(capability);
}

auto ShaderCapabilityConfig::accessCapability(Capability capability) const -> code::Value
{
    return capabilityAccessors.at(capability);
}

auto ShaderCapabilityConfig::getCapabilityType(Capability capability) const -> BasicType
{
    return capabilityTypes.at(capability);
}

auto ShaderCapabilityConfig::getCapabilityResources(Capability capability) const
    -> std::vector<ResourceID>
{
    auto& res = requiredResources.at(capability);
    return { res.begin(), res.end() };
}

} // namespace trc
