#include "builtins.h"

#include <cassert>

#include <format>
#include <ranges>
#include <unordered_map>

#include <trc/material/FragmentShader.h>
#include <trc/material/VertexShader.h>
#include <trc/material/ShaderFunctions.h>



namespace cloth
{

BuiltinProvider::BuiltinProvider(
    std::vector<std::pair<Builtin, BuiltinValueFactory>> _builtinDefinitions)
    :
    builtinDefinitions(
        _builtinDefinitions
            | std::views::transform([](const auto& pair){ return pair.first; })
            | std::views::transform([](const auto& b){ return std::make_pair(b.fullId, b); })
            | std::ranges::to<std::unordered_map>()
    ),
    builtinFactories(
        _builtinDefinitions
            | std::views::transform([](const auto& pair){
                return std::make_pair(pair.first.fullId, pair.second);
            })
            | std::ranges::to<std::unordered_map>()
    )
{
}

auto BuiltinProvider::getDefinition(const FullId& id) -> const Builtin*
{
    auto it = builtinDefinitions.find(id.id);
    if (it != builtinDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

auto BuiltinProvider::getAllDefinitions() const -> std::generator<const Builtin&>
{
    co_yield std::ranges::elements_of(std::views::values(builtinDefinitions));
}

bool BuiltinProvider::validateArgs(
    const Builtin& builtin,
    const std::vector<Builtin::ArgValue>& args)
{
    // Check number of arguments
    if (builtin.args.size() != args.size()) {
        return false;
    }

    // Check argument types
    for (const auto& [def, arg] : std::views::zip(builtin.args, args))
    {
        switch (def)
        {
        case Builtin::ArgType::eValue:
            if (!std::holds_alternative<trc::shader::code::Value>(arg)) return false;
            break;
        case Builtin::ArgType::eResourcePath:
            if (!std::holds_alternative<trc::AssetPath>(arg)) return false;
            break;
        default:
            std::unreachable();
        };
    }

    return true;
}

auto BuiltinProvider::makeValue(
    const FullId& id,
    const std::vector<Builtin::ArgValue>& args,
    trc::shader::ShaderModuleBuilder& builder)
    -> std::expected<trc::shader::code::Value, BuiltinImplementationError>
{
    using E = BuiltinImplementationError;
    using C = BuiltinImplementationError::Code;

    if (!getDefinition(id)) {
        return std::unexpected(E{ C::eNotFound, std::format("\"{}\" is not a Cloth built-in.", id.id) });
    }
    if (!builtinFactories.contains(id.id)) {
        return std::unexpected(E{ C::eNotImplemented, "Requested builtin is not implemented" });
    }

    assert(validateArgs(*getDefinition(id), args) && "Should be verified externally.");
    return builtinFactories.at(id.id)(args, builder);
}



////////////////////////////////////////////////////////////////////////////////
//  Vertex module implementation                                              //
////////////////////////////////////////////////////////////////////////////////

constexpr auto capabilityAccess = [](const trc::shader::Capability& cap) {
    return [cap](const auto&, trc::shader::ShaderModuleBuilder& builder) {
        return builder.makeCapabilityAccess(cap);
    };
};

constexpr auto constantDecl = [](trc::shader::BasicType type) {
    return [type](auto&&, trc::shader::ShaderModuleBuilder& builder) {
        return builder.makeConstant(type);
    };
};

constexpr auto makeTextureAccess = [](trc::shader::ShaderModuleBuilder& builder,
                                      const trc::AssetPath& path)
{
    return builder.makeArrayAccess(
        builder.makeCapabilityAccess(trc::MaterialCapability::kTextureSample),
        builder.makeSpecializationConstant(
            std::make_shared<trc::RuntimeTextureIndex>(path)
        )
    );
};

auto makeVertexBuiltinProvider() -> BuiltinProvider
{
    return BuiltinProvider({
        {
            Builtin{
                .fullId = "vertexPosition",
                .type = glm::vec3{},
            },
            capabilityAccess(trc::VertexCapability::kPosition)
        },
        {
            Builtin{
                .fullId = "vertexNormal",
                .type = glm::vec3{},
            },
            capabilityAccess(trc::VertexCapability::kNormal)
        },
        {
            Builtin{
                .fullId = "vertexUV",
                .type = glm::vec2{},
            },
            capabilityAccess(trc::VertexCapability::kUV)
        },
        {
            Builtin{
                .fullId = "texture",
                .type = glm::vec4{},
                .args{ Builtin::ArgType::eResourcePath },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder) {
                return makeTextureAccess(builder, std::get<trc::AssetPath>(args[0]));
            }
        },
        {
            Builtin{
                .fullId = "tangentSpaceToWorldSpace",
                .type = glm::vec3{},
                .args{ Builtin::ArgType::eValue },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeCall<trc::TangentToWorldspace>({
                    std::get<trc::shader::code::Value>(args[0])
                });
            }
        },
        {
            Builtin{
                .fullId = "sampleTexture",
                .type = glm::vec4{},
                .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeExternalCall("texture", {
                    makeTextureAccess(builder, std::get<trc::AssetPath>(args[0])),
                    std::get<trc::shader::code::Value>(args[1])
                });
            }
        },
        {
            Builtin{
                .fullId = "sampleNormalMap",
                .type = glm::vec3{},
                .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeCall<trc::TangentToWorldspace>({
                    builder.makeMemberAccess(
                        builder.makeExternalCall("texture", {
                            makeTextureAccess(builder, std::get<trc::AssetPath>(args[0])),
                            std::get<trc::shader::code::Value>(args[1])
                        }),
                        "xyz"
                    )
                });
            }
        },

        {
            Builtin{
                .fullId = "out:position",
                .type = glm::vec3{},
            },
            constantDecl(glm::vec3{})
        },
        {
            Builtin{
                .fullId = "out:normal",
                .type = glm::vec3{},
            },
            constantDecl(glm::vec3{})
        },
        {
            Builtin{
                .fullId = "modelMatrix",
                .type = glm::mat4{},
            },
            capabilityAccess(trc::VertexCapability::kModelMatrix)
        },
        {
            Builtin{
                .fullId = "viewProjMatrix",
                .type = glm::mat4{},
            },
            [](const auto&, trc::shader::ShaderModuleBuilder& builder) {
                return builder.makeMul(
                    builder.makeCapabilityAccess(trc::VertexCapability::kProjMatrix),
                    builder.makeCapabilityAccess(trc::VertexCapability::kViewMatrix)
                );
            }
        },
    });
}


////////////////////////////////////////////////////////////////////////////////
//  Fragment module implementation                                            //
////////////////////////////////////////////////////////////////////////////////

auto makeFragmentBuiltinProvider() -> BuiltinProvider
{
    return BuiltinProvider({
        {
            Builtin{
                .fullId = "vertexPosition",
                .type = glm::vec3{},
            },
            capabilityAccess(trc::MaterialCapability::kVertexWorldPos)
        },
        {
            Builtin{
                .fullId = "vertexNormal",
                .type = glm::vec3{},
            },
            capabilityAccess(trc::MaterialCapability::kVertexNormal)
        },
        {
            Builtin{
                .fullId = "vertexUV",
                .type = glm::vec2{},
            },
            capabilityAccess(trc::MaterialCapability::kVertexUV)
        },
        {
            Builtin{
                .fullId = "cameraWorldPos",
                .type = glm::vec3{},
            },
            capabilityAccess(trc::MaterialCapability::kCameraWorldPos)
        },
        {
            Builtin{
                .fullId = "texture",
                .type = glm::vec4{},
                .args{ Builtin::ArgType::eResourcePath },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder) {
                return makeTextureAccess(builder, std::get<trc::AssetPath>(args[0]));
            }
        },
        {
            Builtin{
                .fullId = "tangentSpaceToWorldSpace",
                .type = glm::vec3{},
                .args{ Builtin::ArgType::eValue },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeCall<trc::TangentToWorldspace>({
                    std::get<trc::shader::code::Value>(args[0])
                });
            }
        },
        {
            Builtin{
                .fullId = "sampleTexture",
                .type = glm::vec4{},
                .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeExternalCall("texture", {
                    makeTextureAccess(builder, std::get<trc::AssetPath>(args[0])),
                    std::get<trc::shader::code::Value>(args[1])
                });
            }
        },
        {
            Builtin{
                .fullId = "sampleNormalMap",
                .type = glm::vec3{},
                .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
            },
            [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
            {
                return builder.makeCall<trc::TangentToWorldspace>({
                    builder.makeMemberAccess(
                        builder.makeExternalCall("texture", {
                            makeTextureAccess(builder, std::get<trc::AssetPath>(args[0])),
                            std::get<trc::shader::code::Value>(args[1])
                        }),
                        "xyz"
                    )
                });
            }
        },

        ////////////////////////////
        // Special output parameters
        //
        // Output parameters are implemented as simple variable declarations:
        //
        //     $out:normal = $vertexNormal;
        //
        //              v
        //
        //     vec3 _id_0 = vec3(0, 0, 0);
        //     _id_0 = $vertexNormal;
        //     ...
        //     _normal_output_location = _id_0;
        {
            Builtin{
                .fullId = "out:color",
                .type = glm::vec4{},
            },
            constantDecl(glm::vec4{})
        },
        {
            Builtin{
                .fullId = "out:normal",
                .type = glm::vec3{},
            },
            constantDecl(glm::vec3{})
        },
        {
            Builtin{
                .fullId = "out:specularFactor",
                .type = float{},
            },
            constantDecl(float{})
        },
        {
            Builtin{
                .fullId = "out:roughness",
                .type = float{},
            },
            constantDecl(float{})
        },
        {
            Builtin{
                .fullId = "out:metallicness",
                .type = float{},
            },
            constantDecl(float{})
        },
        {
            Builtin{
                .fullId = "out:emissive",
                .type = bool{},
            },
            constantDecl(bool{})
        },
    });
}

} // namespace cloth
