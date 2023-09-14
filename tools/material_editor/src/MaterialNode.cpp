#include "MaterialNode.h"



class ShaderFuncImpl : public trc::ShaderFunction
{
public:
    ShaderFuncImpl(const NodeDescription& desc)
        :
        trc::ShaderFunction(desc.technicalName, makeShaderFunctionSignature(desc)),
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
        .technicalName = func->getName(),
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
