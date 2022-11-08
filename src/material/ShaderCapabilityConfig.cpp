#include "trc/material/ShaderCapabilityConfig.h"

#include <stdexcept>



namespace trc
{

auto ShaderCapabilityConfig::addResource(Resource shaderResource) -> ResourceID
{
    resources.push_back({ std::move(shaderResource), {}, {} });
    return resources.size() - 1;
}

void ShaderCapabilityConfig::addShaderExtension(ResourceID resource, std::string extensionName)
{
    resources.at(resource).extensions.emplace(std::move(extensionName));
}

void ShaderCapabilityConfig::addShaderInclude(ResourceID resource, util::Pathlet includePath)
{
    resources.at(resource).includeFiles.emplace(std::move(includePath));
}

void ShaderCapabilityConfig::linkCapability(Capability capability, ResourceID resource)
{
    assert(resource < resources.size());

    auto [_, success] = resourceFromCapability.try_emplace(capability, resource);
    if (!success) {
        throw std::invalid_argument("[In ShaderCapabilityConfig::linkCapability]: The capability"
                                    " is already linked to a resource!");
    }
}

bool ShaderCapabilityConfig::hasCapability(Capability capability) const
{
    return resourceFromCapability.contains(capability);
}

auto ShaderCapabilityConfig::getResource(Capability capability) const
    -> std::optional<const ResourceData*>
{
    auto it = resourceFromCapability.find(capability);
    if (it != resourceFromCapability.end()) {
        return &resources.at(it->second);
    }
    return std::nullopt;
}

void ShaderCapabilityConfig::setCapabilityAccessor(Capability capability, std::string accessorCode)
{
    capabilityAccessors[capability] = std::move(accessorCode);
}

auto ShaderCapabilityConfig::getCapabilityAccessor(Capability capability) const
    -> std::optional<std::string>
{
    auto it = capabilityAccessors.find(capability);
    if (it != capabilityAccessors.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace trc
