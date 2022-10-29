#include "trc/material/MaterialCompiler.h"



namespace trc
{

MaterialCompiler::MaterialCompiler(ShaderCapabilityConfig config)
    :
    config(std::move(config))
{
}

auto MaterialCompiler::compile(MaterialOutputNode& outNode) -> MaterialCompileResult
{
    ShaderResourceInterface resourceCompiler(config);
    const auto functionCode = compileFunctions(resourceCompiler, outNode);
    const auto resources = resourceCompiler.compile();

    std::stringstream ss;
    ss << "#version 460\n";
    ss << "#extension GL_EXT_nonuniform_qualifier : require\n";
    ss << "\n";

    // Write resources
    ss << resources.getGlslCode() << "\n";

    // Write output variables
    for (const auto& [location, type] : outNode.getOutputs())
    {
        ss << "layout (location = " << location << ") out " << type.to_string()
           << " fragmentOutput_" << location << ";\n";
    }
    ss << "\n";

    // Write functions
    ss << std::move(functionCode) << "\n";

    // Write main
    ss << "void main()\n{\n";
    for (const auto& link : outNode.getOutputLinks())
    {
        auto paramNode = outNode.getParameter(link.param);
        if (paramNode != nullptr)
        {
            const auto& [location, _] = outNode.getOutput(link.output);
            ss << "fragmentOutput_" << location << link.outputAccessor
               << " = " << call(paramNode) << ";\n";
        }
    }
    ss << "\n}";

    return {
        .fragmentGlslCode=ss.str(),
        .requiredTextures=resources.getReferencedTextures(),
    };
}

auto MaterialCompiler::compileFunctions(
    ShaderResourceInterface& resources,
    MaterialOutputNode& mat) -> std::string
{
    std::unordered_map<std::string, MaterialFunction*> functions;

    // Recursive helper function to traverse the graph
    std::function<void(MaterialNode*)> collectFunctions = [&](MaterialNode* node)
    {
        for (auto inputNode : node->getInputs())
        {
            assert(inputNode != nullptr);
            collectFunctions(inputNode);
        }
        functions.try_emplace(node->getFunction().getSignature().name, &node->getFunction());
    };

    // Collect all functions in the graph
    for (auto node : mat.getParameters())
    {
        if (node != nullptr) {
            collectFunctions(node);
        }
    }

    // Compile functions to GLSL code
    std::stringstream ss;
    for (const auto& [_, func] : functions)
    {
        const auto& sig = func->getSignature();
        ss << sig.output.type.to_string() << " " << sig.name << "(";
        for (ui32 i = 0; i < sig.inputs.size(); ++i)
        {
            ss << sig.inputs.at(i).type.to_string() << " " << sig.inputs.at(i).name;
            if (i < sig.inputs.size() - 1) {
                ss << ", ";
            }
        }
        ss << ")\n{\n" << func->makeGlslCode(resources) << "\n}\n";
    }

    return ss.str();
}

auto MaterialCompiler::call(MaterialNode* node) -> std::string
{
    assert(node != nullptr);

    const auto& sig = node->getFunction().getSignature();
    std::stringstream ss;
    ss << sig.name << "(";
    for (ui32 i = 0; i < sig.inputs.size(); ++i)
    {
        ss << call(node->getInputs().at(i));
        if (i < sig.inputs.size() - 1) {
            ss << ", ";
        }
    }
    ss << ")";

    return ss.str();
}

} // namespace trc
