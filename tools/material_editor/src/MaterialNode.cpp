#include "MaterialNode.h"

#include <array>
#include <concepts>

#include <trc/material/CommonShaderFunctions.h>
#include <trc/material/FragmentShader.h>



template<std::derived_from<trc::ShaderFunction> F>
auto makeBuilder(F&& func)
{
    return [func=std::move(func)](trc::ShaderModuleBuilder& builder,
                                  const std::vector<code::Value>& args) mutable
    {
        return builder.makeCall(func, args);
    };
}

auto makeNodeDescription(s_ptr<trc::ShaderFunction> func) -> NodeDescription
{
    constexpr const char* noDescription = "no description :(";

    NodeDescription desc{
        .name = func->getName(),
        .id = func->getName(),
        .description = noDescription,
        .computation = {
            .arguments{},
            .argumentTypes{},
            .typeCorrelatedArguments{},
            .builder=[func](trc::ShaderModuleBuilder& builder, std::vector<code::Value> args)
                -> code::Value
            {
                return builder.makeCall(*func, std::move(args));
            },
        },
    };

    for (ui32 i = 0; auto inType : func->getType().argTypes)
    {
        desc.computation.arguments.emplace_back("arg" + std::to_string(i++), noDescription);
        desc.computation.argumentTypes.emplace_back(inType);
    }
    if (auto outType = func->getType().returnType) {
        desc.computation.resultType = *outType;
    }

    return desc;
}

auto getMaterialNodes() -> const std::unordered_map<std::string, NodeDescription>&
{
    static std::unordered_map<std::string, NodeDescription> allNodes{
        { getOutputNode().id, getOutputNode() },
        { "matedit_fun_white", NodeDescription{
            .name = "White",
            .id = "matedit_fun_white",
            .description = "Just a while color",
            .computation = {
                .resultType=vec4{},
                .builder=[](trc::ShaderModuleBuilder& builder, auto&&) {
                    return builder.makeConstant(vec4(1.0f));
                },
            },
        }},
        { "matedit_fun_mix", NodeDescription{
            .name = "Mix",
            .id = "matedit_fun_mix",
            .description = "Linearly interpolate two values",
            .computation = {
                .arguments{
                    { "Value A", "A numerical value." },
                    { "Value B", "Another numerical value." },
                    { "Factor", "The interpolation factor with which the values are combined." },
                },
                .argumentTypes{ TypeRange::makeMax(vec4{}), TypeRange::makeMax(vec4{}), float{} },
                .resultInfluencingArgs{ 0, 1 },
                .builder=makeBuilder(trc::Mix<4, float>{}),
            },
        }},
    };

    return allNodes;
}

auto getOutputNode() -> const NodeDescription&
{
    static constexpr std::array<trc::BasicType, 6> paramTypes{
        vec4{},   // color
        vec3{},   // normal
        float{},  // specular factor ('shinyness')
        float{},  // roughness
        float{},  // metallicness
        float{},  // emissive
    };

    static const NodeDescription outputNode{
        .name = "Material Parameters",
        .id = "matedit_canonical_output_node",
        .description = "The output parameters of the material.",
        .computation = {
            .arguments{
                { "Albedo", "A base color value." },
                { "Normal", "The shaded object's surface normal." },
                { "Specular Factor (shinyness)", "A factor that scales the sharpness of specular reflections." },
                { "Roughness", "A factor that scales how rough the material looks." },
                { "Metallicness", "A factor that scales how metallic-ish the material looks. Note: Currently not implemented." },
                { "Emissive", "Is the material emissive?" },
            },
            .argumentTypes{
                TypeRange::makeMax(paramTypes[0]),
                TypeRange::makeMax(paramTypes[1]),
                TypeRange::makeMax(paramTypes[2]),
                TypeRange::makeMax(paramTypes[3]),
                TypeRange::makeMax(paramTypes[4]),
                TypeRange::makeMax(paramTypes[5]),
            },
        },
    };

    return outputNode;
}
