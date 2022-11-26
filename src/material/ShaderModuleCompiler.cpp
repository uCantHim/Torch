#include "trc/material/ShaderModuleCompiler.h"

#include "trc/material/ShaderCodeCompiler.h"



namespace trc
{

ShaderModule::ShaderModule(
    std::string shaderCode,
    ShaderResources resourceInfo,
    std::unordered_map<ParameterID, std::string> paramResultVariableNames,
    std::string outputReplacementVariableName)
    :
    ShaderResources(std::move(resourceInfo)),
    shaderGlslCode(std::move(shaderCode)),
    paramResultVariableNames(std::move(paramResultVariableNames)),
    outputReplacementVariableName(std::move(outputReplacementVariableName))
{
}

auto ShaderModule::getGlslCode() const -> const std::string&
{
    return shaderGlslCode;
}

auto ShaderModule::getParameterName(ParameterID paramNode) const
    -> std::optional<std::string>
{
    if (!paramResultVariableNames.contains(paramNode)) {
        return std::nullopt;
    }
    return paramResultVariableNames.at(paramNode);
}

auto ShaderModule::getOutputPlaceholderVariableName() const -> std::string
{
    return outputReplacementVariableName;
}



auto ShaderModuleCompiler::compile(ShaderOutputNode& outNode, ShaderModuleBuilder& builder)
    -> ShaderModule
{
    ShaderValueCompiler codeCompiler;

    // Write main
    std::stringstream main;
    main << "void main()\n{\n";

    for (const auto& link : outNode.getOutputLinks())
    {
        auto value = outNode.getParameter(link.param);
        if (value == nullptr) continue;

        // This call generates code from the graph
        auto [id, code] = codeCompiler.compile(value);
        main << code;

        // Generate assignment to output location
        const auto& [location, _] = outNode.getOutput(link.output);
        main << "shaderOutput_" << location << link.outputAccessor << " = " << id << ";\n";
    }

    // Write assignments to builtin variables (gl_Position, gl_FragDepth, ...)
    for (const auto& [varName, param] : outNode.getBuiltinOutputs())
    {
        auto value = outNode.getParameter(param);
        if (value != nullptr)
        {
            auto [id, code] = codeCompiler.compile(value);
            main << code;
            main << varName << " = " << id << ";\n";
        }
    }
    main << "\n}";

    // Generate resource and function declarations
    auto functionDeclCode = builder.compileFunctionDecls();
    auto resources = builder.compileResourceDecls();

    // Build shader file
    std::stringstream ss;
    ss << "#version 460\n";

    // Write resources
    ss << resources.getGlslCode() << "\n";

    // Write function definitions
    ss << std::move(functionDeclCode) << "\n";

    // Write output variables
    for (const auto& [location, type] : outNode.getOutputs())
    {
        ss << "layout (location = " << location << ") out " << type.to_string()
           << " shaderOutput_" << location << ";\n";
    }
    ss << "\n";

    ss << main.str();

    return {
        ss.str(),
        resources,
        {},
        "SHADER_OUTPUT_PLACEHOLDER"
    };
}

} // namespace trc
