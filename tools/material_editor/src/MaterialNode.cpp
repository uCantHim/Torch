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
        .inputs{},
        .inputTypeCorrelations{},
        .outputs = { ComputedValue{
            .resultType=func->getType().returnType,
            .builder=[func](trc::ShaderModuleBuilder& builder, std::vector<code::Value> args)
                -> code::Value
            {
                return builder.makeCall(*func, std::move(args));
            },
        }},
    };

    for (ui32 i = 0; auto inType : func->getType().argTypes)
    {
        desc.inputs.emplace_back("arg" + std::to_string(i++),
                                 noDescription,
                                 TypeRange::makeEq(inType));
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
            .outputs = { ComputedValue{
                .resultType=vec4{},
                .builder=[](trc::ShaderModuleBuilder& builder, auto&&) {
                    return builder.makeConstant(vec4(1.0f));
                },
            }},
        }},
        { "matedit_fun_simple_value", NodeDescription{
            .name = "Constant Value",
            .id = "matedit_fun_simple_value",
            .description = "A constant value",
            .outputs = { ComputedValue{
                .resultType=float{},
                .builder=[](trc::ShaderModuleBuilder& builder, auto&&) {
                    return builder.makeConstant(0.0f);
                },
            }},
        }},
        { "matedit_fun_float_value", NodeDescription{
            .name = "Float",
            .id = "matedit_fun_float_value",
            .description = "A constant floating-point value",
            .outputs = { ConstantValue{
                .value{ 0.0f },
                .type=ConstantValueType::eFloat,
            }},
        }},
        { "matedit_fun_mix", NodeDescription{
            .name = "Mix",
            .id = "matedit_fun_mix",
            .description = "Linearly interpolate two values",
            .inputs{
                { "Value A", "A numerical value.",       TypeRange::makeMax(vec4{}) },
                { "Value B", "Another numerical value.", TypeRange::makeMax(vec4{}) },
                { "Factor", "The interpolation factor with which the values are combined.",
                  float{} },
            },
            .inputTypeCorrelations{ { 0, 1 } },
            .outputs = { ComputedValue{
                .resultInfluencingArgs{ 0, 1 },
                .builder=makeBuilder(trc::Mix<4, float>{}),
            }},
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
        .inputs{
            { "Albedo", "A base color value.",
              TypeRange::makeMax(paramTypes[0]) },
            { "Normal", "The shaded object's surface normal.",
              TypeRange::makeMax(paramTypes[1]) },
            { "Specular Factor (shinyness)", "A factor that scales the sharpness of specular reflections.",
              TypeRange::makeMax(paramTypes[2]) },
            { "Roughness", "A factor that scales how rough the material looks.",
              TypeRange::makeMax(paramTypes[3]) },
            { "Metallicness", "A factor that scales how metallic-ish the material looks. Note: Currently not implemented.",
              TypeRange::makeMax(paramTypes[4]) },
            { "Emissive", "Is the material emissive?",
              TypeRange::makeMax(paramTypes[5]) },
        },
    };

    return outputNode;
}
