#include "trc/material/ShaderModuleCompiler.h"

#include "trc/material/ShaderCodeCompiler.h"
#include "trc/util/TorchDirectories.h"



namespace trc
{

ShaderModule::ShaderModule(
    std::string shaderCode,
    ShaderResources resourceInfo,
    std::unordered_map<ParameterID, std::string> paramResultVariableNames)
    :
    ShaderResources(std::move(resourceInfo)),
    shaderGlslCode(std::move(shaderCode)),
    paramResultVariableNames(std::move(paramResultVariableNames))
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



auto ShaderModuleCompiler::compile(ShaderOutputNode& outNode, ShaderModuleBuilder& builder)
    -> ShaderModule
{
    // Generate assignments of parameter values to output locations
    for (const auto& link : outNode.getOutputLinks())
    {
        auto value = outNode.getParameter(link.param);
        if (value == nullptr) continue;

        // Generate assignment to output location
        const auto& [location, _] = outNode.getOutput(link.output);
        builder.makeAssignment(
            builder.makeExternalIdentifier(
                "shaderOutput_" + std::to_string(location) + link.outputAccessor
            ),
            value
        );
    }

    // Generate resource and function declarations
    auto functionDeclCode = builder.compileFunctionDecls();
    auto resources = builder.compileResourceDecls();

    // Build shader file
    std::stringstream ss;
    ss << "#version 460\n";

    // Write resources
    ss << resources.getGlslCode() << "\n";

    // Write additional includes
    spirv::FileIncluder includer(
        util::getInternalShaderBinaryDirectory(),
        { util::getInternalShaderStorageDirectory() }
    );
    ss << builder.compileIncludedCode(includer);

    // Write shader output locations
    for (const auto& [location, type] : outNode.getOutputs())
    {
        ss << "layout (location = " << location << ") out " << type.to_string()
           << " shaderOutput_" << location << ";\n";
    }
    ss << "\n";

    // Write function definitions
    // This also writes the main function.
    ss << std::move(functionDeclCode) << "\n";

    return {
        ss.str(),
        resources,
        {}
    };
}

} // namespace trc
