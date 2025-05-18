#include "builtins.h"

#include <cassert>

#include <format>
#include <ranges>
#include <unordered_map>

#include <trc/material/FragmentShader.h>
#include <trc/material/ShaderFunctions.h>



namespace cloth
{

auto defineBuiltins(std::initializer_list<Builtin> list)
    -> std::unordered_map<std::string, Builtin>
{
    return std::views::transform(list, [](const Builtin& b){ return std::make_pair(b.fullId, b); })
        | std::ranges::to<std::unordered_map>();
}

auto getBuiltinDefinitions() -> auto&
{
    static auto res = defineBuiltins({
        Builtin{
            .fullId = "vertexNormal",
            //.name = "Vertex Normal",
            //.description = "The interpolated vertex normal at the current fragment position.",
            .type = glm::vec3{},
        },
        Builtin{
            .fullId = "vertexUV",
            .type = glm::vec2{},
        },
        Builtin{
            .fullId = "cameraWorldPos",
            .type = glm::vec3{},
        },
        Builtin{
            .fullId = "texture",
            .type = glm::vec4{},
            .args{ Builtin::ArgType::eResourcePath },
        },
        Builtin{
            .fullId = "tangentSpaceToWorldSpace",
            .type = glm::vec3{},
            .args{ Builtin::ArgType::eValue },
        },
        Builtin{
            .fullId = "sampleTexture",
            .type = glm::vec4{},
            .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
        },
        Builtin{
            .fullId = "sampleNormalMap",
            .type = glm::vec3{},
            .args{ { Builtin::ArgType::eResourcePath, Builtin::ArgType::eValue } },
        },

        ////////////////////////////
        // Special output parameters
        Builtin{
            .fullId = "out:color",
            .type = glm::vec4{},
        },
        Builtin{
            .fullId = "out:normal",
            .type = glm::vec3{},
        },
        Builtin{
            .fullId = "out:specularFactor",
            .type = float{},
        },
        Builtin{
            .fullId = "out:roughness",
            .type = float{},
        },
        Builtin{
            .fullId = "out:metallicness",
            .type = float{},
        },
        Builtin{
            .fullId = "out:emissive",
            .type = bool{},
        },
    });
    return res;
}

auto getBuiltinFactories() -> auto&
{
    using BuiltinValueFactory = std::function<trc::shader::code::Value(
        const std::vector<Builtin::ArgValue>&,
        trc::shader::ShaderModuleBuilder&
    )>;

    static constexpr auto capabilityAccess = [](const trc::shader::Capability& cap) {
        return [cap](const auto&, trc::shader::ShaderModuleBuilder& builder) {
            return builder.makeCapabilityAccess(cap);
        };
    };
    static constexpr auto constantDecl = [](trc::shader::BasicType type) {
        return [type](auto&&, trc::shader::ShaderModuleBuilder& builder) {
            return builder.makeConstant(type);
        };
    };

    static auto factories = []{
        static constexpr auto makeTextureAccess = [](trc::shader::ShaderModuleBuilder& builder,
                                                     const trc::AssetPath& path)
        {
            return builder.makeArrayAccess(
                builder.makeCapabilityAccess(trc::MaterialCapability::kTextureSample),
                builder.makeSpecializationConstant(
                    std::make_shared<trc::RuntimeTextureIndex>(path)
                )
            );
        };

        std::unordered_map<std::string, BuiltinValueFactory> res{
            { "vertexPosition", capabilityAccess(trc::MaterialCapability::kVertexWorldPos) },
            { "vertexNormal", capabilityAccess(trc::MaterialCapability::kVertexNormal) },
            { "vertexUV", capabilityAccess(trc::MaterialCapability::kVertexUV) },
            { "cameraWorldPos", capabilityAccess(trc::MaterialCapability::kCameraWorldPos) },
            {
                "texture",
                [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder) {
                    return makeTextureAccess(builder, std::get<trc::AssetPath>(args[0]));
                }
            },
            {
                "tangentSpaceToWorldSpace",
                [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
                {
                    return builder.makeCall<trc::TangentToWorldspace>({
                        std::get<trc::shader::code::Value>(args[0])
                    });
                }
            },
            {
                "sampleTexture",
                [](const std::vector<Builtin::ArgValue>& args, trc::shader::ShaderModuleBuilder& builder)
                {
                    return builder.makeExternalCall("texture", {
                        makeTextureAccess(builder, std::get<trc::AssetPath>(args[0])),
                        std::get<trc::shader::code::Value>(args[1])
                    });
                }
            },
            {
                "sampleNormalMap",
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
            { "out:color", constantDecl(glm::vec4{}) },
            { "out:normal", constantDecl(glm::vec3{}) },
            { "out:specularFactor", constantDecl(float{}) },
            { "out:metallicness", constantDecl(float{}) },
            { "out:roughness", constantDecl(float{}) },
            { "out:emissive", constantDecl(bool{}) },
        };
        return res;
    }();
    return factories;
}



auto BuiltinProvider::getDefinition(const FullId& id) -> const Builtin*
{
    auto it = getBuiltinDefinitions().find(id.id);
    if (it != getBuiltinDefinitions().end()) {
        return &it->second;
    }
    return nullptr;
}

auto BuiltinProvider::getAllDefinitions() const -> std::generator<const Builtin&>
{
    co_yield std::ranges::elements_of(std::views::values(getBuiltinDefinitions()));
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
    if (!getBuiltinFactories().contains(id.id)) {
        return std::unexpected(E{ C::eNotImplemented, "Requested builtin is not implemented" });
    }

    assert(validateArgs(*getDefinition(id), args) && "Should be verified externally.");
    return getBuiltinFactories().at(id.id)(args, builder);
}

} // namespace cloth
