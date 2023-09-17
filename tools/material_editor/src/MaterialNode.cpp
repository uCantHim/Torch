#include "MaterialNode.h"

#include <trc/material/CommonShaderFunctions.h>



class ShaderFuncImpl : public trc::ShaderFunction
{
public:
    ShaderFuncImpl(const NodeDescription& desc)
        :
        trc::ShaderFunction(desc.id, makeShaderFunctionSignature(desc)),
        func(desc.computation)
    {}

    void build(trc::ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        builder.makeReturn(func(builder, std::move(args)));
    }

private:
    NodeComputationBuilder func;
};



auto makeShaderFunctionSignature(const NodeDescription& desc) -> trc::FunctionType
{
    trc::FunctionType sig;
    for (auto& in : desc.inputs) {
        sig.argTypes.emplace_back(in.type);
    }
    if (desc.output) {
        sig.returnType = desc.output->type;
    }
    return sig;
}

auto makeShaderFunction(const NodeDescription& nodeDesc) -> s_ptr<trc::ShaderFunction>
{
    return std::make_shared<ShaderFuncImpl>(nodeDesc);
}

auto makeNodeDescription(s_ptr<trc::ShaderFunction> func) -> NodeDescription
{
    constexpr const char* noDescription = "no description :(";

    NodeDescription desc{
        .name = func->getName(),
        .id = func->getName(),
        .description = noDescription,
        .computation = [func](trc::ShaderModuleBuilder& builder, std::vector<code::Value> args)
            -> code::Value
        {
            return builder.makeCall(*func, std::move(args));
        },
        .inputs{},
        .output{}
    };

    ui32 i{ 0 };
    for (auto inType : func->getType().argTypes) {
        desc.inputs.push_back({ inType, "arg" + std::to_string(i++), noDescription });
    }
    if (auto outType = func->getType().returnType) {
        desc.output = { *outType, "out", noDescription };
    }

    return desc;
}

auto getMaterialNodes() -> const std::unordered_map<std::string, NodeDescription>&
{
    static std::unordered_map<std::string, NodeDescription> allNodes{
        { "matedit_fun_white", NodeDescription{
            .name = "White",
            .id = "matedit_fun_white",
            .description = "Just a while color",
            .computation = [](trc::ShaderModuleBuilder& builder, auto&&) {
                return builder.makeConstant(vec4(1.0f));
            },
            .inputs{},
            .output = NodeValue{ vec4{}, "Color", "A color value" },
        }},
        { "matedit_fun_mix", NodeDescription{
            .name = "Mix",
            .id = "matedit_fun_mix",
            .description = "Linearly interpolate two values",
            .computation = [](trc::ShaderModuleBuilder& builder, auto&& args) {
                return builder.makeAdd(
                    builder.makeMul(args[0], args[2]),
                    builder.makeMul(args[1], builder.makeSub(builder.makeConstant(1.0f), args[2]))
                );
            },
            .inputs{ { vec4{}, "Color A", "" }, { vec4{}, "Color B", "" }, { float{}, "Factor", "" } },
            .output = NodeValue{ vec4{}, "Color", "A color value" },
        }},
    };

    return allNodes;
}
