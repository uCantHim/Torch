#include "trc/material/ShaderCapabilityConfig.h"

#include <stdexcept>



namespace trc
{

const std::array<
    ShaderCapabilityConfig::BuiltinConstantInfo,
    static_cast<size_t>(Builtin::eNumBuiltinConstants)
> ShaderCapabilityConfig::builtinConstants{{
    { vec3{},  Capability::eVertexPosition,  },  // "VertexPos",    "worldPos", {},
    { vec3{},  Capability::eVertexNormal,    },  // "VertexNormal", "tbn[2]",   {},
    { vec2{},  Capability::eVertexUV,        },  // "VertexUVs",    "uv",       {},

    { float{}, Capability::eTime,            },  // "Time",      "totalTime", {},
    { float{}, Capability::eTimeDelta,       },  // "TimeDelta", "timeDelta", {},
}};

auto ShaderCapabilityConfig::addResource(Resource shaderResource) -> ResourceID
{
    resources.emplace_back(std::move(shaderResource));
    return resources.size() - 1;
}

void ShaderCapabilityConfig::linkCapability(Capability capability, ResourceID resource)
{
    assert(resource < resources.size());
    resourceFromCapability.try_emplace(capability, resource);
}

bool ShaderCapabilityConfig::hasCapability(Capability capability) const
{
    return resourceFromCapability.contains(capability);
}

auto ShaderCapabilityConfig::getResource(Capability capability) const
    -> std::optional<const Resource*>
{
    auto it = resourceFromCapability.find(capability);
    if (it != resourceFromCapability.end()) {
        return &resources.at(it->second);
    }
    return std::nullopt;
}

void ShaderCapabilityConfig::setConstantAccessor(Builtin constant, std::string accessorCode)
{
    accessorFromBuiltinConstant.try_emplace(constant, std::move(accessorCode));
}

auto ShaderCapabilityConfig::getConstantAccessor(Builtin constant) const
    -> std::optional<std::string>
{
    auto it = accessorFromBuiltinConstant.find(constant);
    if (it != accessorFromBuiltinConstant.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto ShaderCapabilityConfig::getConstantCapability(Builtin constant) -> Capability
{
    return builtinConstants[static_cast<size_t>(constant)].capability;
}

auto ShaderCapabilityConfig::getConstantType(Builtin constant) -> BasicType
{
    return builtinConstants[static_cast<size_t>(constant)].type;
}

} // namespace trc
