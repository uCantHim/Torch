#include "trc/material/shader/CapabilityConfig.h"

#include <stdexcept>

#include "trc/material/shader/ShaderModuleBuilder.h"



namespace trc::shader
{

void CapabilityConfig::addGlobalShaderExtension(std::string extensionName)
{
    globalExtensions.emplace(std::move(extensionName));
}

void CapabilityConfig::addGlobalShaderInclude(util::Pathlet includePath)
{
    globalIncludes.emplace(std::move(includePath));
}

auto CapabilityConfig::getGlobalShaderExtensions() const
    -> const std::unordered_set<std::string>&
{
    return globalExtensions;
}

auto CapabilityConfig::getGlobalShaderIncludes() const
    -> const std::unordered_set<util::Pathlet>&
{
    return globalIncludes;
}

auto CapabilityConfig::addResource(Resource shaderResource) -> ResourceID
{
    const ResourceID id{ static_cast<ui32>(resources.size()) };
    const std::string name = "_access_resource_" + std::to_string(id);
    resources.emplace_back(new ResourceData{ std::move(shaderResource), name, {}, {}, {} });

    return id;
}

void CapabilityConfig::addShaderExtension(ResourceID resource, std::string extensionName)
{
    resources.at(resource)->extensions.emplace(std::move(extensionName));
}

void CapabilityConfig::addShaderInclude(ResourceID resource, util::Pathlet includePath)
{
    resources.at(resource)->includeFiles.emplace(std::move(includePath));
}

void CapabilityConfig::addMacro(
    ResourceID resource,
    std::string name,
    std::optional<std::string> value)
{
    resources.at(resource)->macroDefinitions[std::move(name)] = std::move(value);
}

auto CapabilityConfig::accessResource(ResourceID resourceId, ShaderModuleBuilder& builder) const
    -> code::Value
{
    auto& resource = getResourceInfo(resourceId);
    return builder.makeExternalIdentifier(resource.resourceMacroName);
}

auto CapabilityConfig::getResourceInfo(ResourceID resource) const -> const ResourceData&
{
    assert(resource < resources.size());
    return *resources.at(resource);
}

auto CapabilityConfig::makeCapabilityBuilderFromResource(ResourceID resource)
    -> CapabilityBuilder
{
    // More verbose than a lambda but better to debug.
    struct ResourceAccessCapabilityBuilder
    {
        auto operator()(CapabilityBuildContext& ctx) -> code::Value {
            return ctx.accessResource(resource);
        }
        ResourceID resource;
    };

    return ResourceAccessCapabilityBuilder{ resource };
}

void CapabilityConfig::linkCapability(Capability capability, ResourceID resource)
{
    return linkCapability(capability, makeCapabilityBuilderFromResource(resource));
}

void CapabilityConfig::linkCapability(Capability capability, CapabilityBuilder impl)
{
    capabilityImpls[capability] = std::move(impl);
}

bool CapabilityConfig::hasCapability(Capability capability) const
{
    return capabilityImpls.contains(capability);
}

auto CapabilityConfig::accessCapability(Capability capability, ShaderModuleBuilder& builder) const
    -> std::pair<code::Value, std::vector<ResourceID>>
{
    auto it = capabilityImpls.find(capability);
    if (it != capabilityImpls.end())
    {
        CapabilityBuildContext ctx{ this, builder };
        code::Value value = it->second(ctx);
        return { value, {ctx.accessedResources.begin(), ctx.accessedResources.end()} };
    }

    throw std::out_of_range(std::format(
        "[In CapabilityConfig::accessCapability]: The requested capability \"{}\" is not defined.",
        capability.toString()
    ));
}



auto CapabilityBuildContext::accessResource(CapabilityConfig::ResourceID resourceID)
    -> code::Value
{
    accessedResources.emplace(resourceID);
    return conf->accessResource(resourceID, builder);
}

} // namespace trc::shader
