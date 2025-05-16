#include "builtins.h"

#include <ranges>
#include <unordered_map>

#include <trc/material/FragmentShader.h>



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
            .args{ { Builtin::ArgType::eTexturePath } },
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
            .fullId = "out:roughness",
            .type = float{},
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
        std::unordered_map<std::string, BuiltinValueFactory> res{
            { "vertexPosition", capabilityAccess(trc::MaterialCapability::kVertexWorldPos) },
            { "vertexNormal", capabilityAccess(trc::MaterialCapability::kVertexNormal) },
            { "vertexUV", capabilityAccess(trc::MaterialCapability::kVertexUV) },
            { "cameraWorldPos", capabilityAccess(trc::MaterialCapability::kCameraWorldPos) },

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
    if (!builtin.args) {
        return args.empty();
    }
    if (builtin.args->size() != args.size()) {
        return false;
    }

    // Check argument types
    for (const auto& [def, arg] : std::views::zip(*builtin.args, args))
    {
        switch (def)
        {
        case Builtin::ArgType::eValue:
            if (!std::holds_alternative<trc::shader::code::Value>(arg)) return false;
            break;
        case Builtin::ArgType::eTexturePath:
            if (!std::holds_alternative<resource_references::Texture>(arg)) return false;
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
        return std::unexpected(E{ C::eNotFound, "Requested builtin is not an input builtin" });
    }
    if (!validateArgs(*getDefinition(id), args)) {
        assert(false);
        return std::unexpected(E{ C::eInvalidArguments, "Given arguments do not match expected arguments" });
    }
    if (!getBuiltinFactories().contains(id.id)) {
        return std::unexpected(E{ C::eNotImplemented, "Requested builtin is not implemented" });
    }

    return getBuiltinFactories().at(id.id)(args, builder);
}

} // namespace cloth
