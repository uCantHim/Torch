#include "trc/material/MaterialCompiler.h"



namespace trc
{

MaterialCompileResult::MaterialCompileResult(
    std::string shaderCode,
    ShaderResources resourceInfo,
    std::unordered_map<ParameterID, std::string> paramResultVariableNames,
    std::string outputReplacementVariableName)
    :
    shaderGlslCode(std::move(shaderCode)),
    resources(std::move(resourceInfo)),
    paramResultVariableNames(std::move(paramResultVariableNames)),
    outputReplacementVariableName(std::move(outputReplacementVariableName))
{
}

auto MaterialCompileResult::getParameterResultVariableName(ParameterID paramNode) const
    -> std::optional<std::string>
{
    if (!paramResultVariableNames.contains(paramNode)) {
        return std::nullopt;
    }
    return paramResultVariableNames.at(paramNode);
}

auto MaterialCompileResult::getOutputPlaceholderVariableName() const -> std::string
{
    return outputReplacementVariableName;
}

auto MaterialCompileResult::getRequiredTextures() const
    -> const std::vector<ShaderResources::TextureResource>&
{
    return resources.getReferencedTextures();
}

auto MaterialCompileResult::getRequiredShaderInputs() const
    -> const std::vector<ShaderResources::ShaderInputInfo>&
{
    return resources.getShaderInputs();
}


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
           << " shaderOutput_" << location << ";\n";
    }
    ss << "\n";

    // Write functions
    ss << std::move(functionCode) << "\n";

    // Write main
    ss << "void main()\n{\n";

    // Write calculated parameters to temporary variables
    std::unordered_map<ParameterID, std::string> paramNames;
    for (ui32 i = 0; const auto& param : outNode.getParameters())
    {
        if (param == nullptr) continue;

        auto type = param->getFunction().getSignature().output.type;
        auto [it, success] = paramNames.try_emplace(i, "materialParamResult_" + std::to_string(i));
        assert(success);
        ++i;

        ss << type.to_string() << " " << it->second << " = " << call(param) << ";\n";
    }

    // Write intermediate parameter values to linked output locations
    for (const auto& link : outNode.getOutputLinks())
    {
        auto paramNode = outNode.getParameter(link.param);
        if (paramNode != nullptr)
        {
            const auto& [location, _] = outNode.getOutput(link.output);
            ss << "shaderOutput_" << location << link.outputAccessor
               << " = " << paramNames.at(link.param) << ";\n";
        }
    }

    // Write footer
    ss << "//$ SHADER_OUTPUT_PLACEHOLDER\n";
    ss << "\n}";

    return {
        ss.str(),
        resources,
        std::move(paramNames),
        "SHADER_OUTPUT_PLACEHOLDER"
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
