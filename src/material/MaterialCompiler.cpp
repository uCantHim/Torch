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
    ShaderResourceInterface resourceInterface(config);
    const auto functionCode = compileFunctions(resourceInterface, outNode);
    const auto resources = resourceInterface.compile();

    std::stringstream ss;
    ss << "#version 460\n";

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

    // Test parameter-subgraphs for cycles
    for (ui32 i = 0; const auto& param : outNode.getParameters())
    {
        if (param == nullptr) continue;
        if (hasCycles(param)) {
            throw std::runtime_error("[In MaterialCompiler::compile]: Detected cycle in node graph"
                                     " of output node's parameter #" + std::to_string(i));
        }
    }

    // Generate code from the graph.
    //
    // Code generation creates single lines of identifier assignments that
    // are written to the stream afterwards. This is done to ensure correct
    // declaration order.
    //
    // Additional results are a map of identifier names to parameters for code
    // post-processing, and an `outputCode` stream that contains assignments
    // from parameter-value identifiers to the final shader output locations.
    // This is written to the stream at the end of the main function.
    std::unordered_map<ParameterID, std::string> paramNames;
    std::stringstream outputCode;
    for (const auto& link : outNode.getOutputLinks())
    {
        auto paramNode = outNode.getParameter(link.param);
        if (paramNode == nullptr) continue;

        // This call generates code from the graph
        const auto id = getResultIdentifier(paramNode);
        paramNames.try_emplace(link.param, id);

        // Generate assignment to output location
        const auto& [location, _] = outNode.getOutput(link.output);
        outputCode << "shaderOutput_" << location << link.outputAccessor << " = " << id << ";\n";
    }

    // Write generated identifier assignments
    for (const auto& str : identifierDecls) {
        ss << str << "\n";
    }

    // Write code that assigns values to shader output locations
    ss << outputCode.str();

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

bool MaterialCompiler::hasCycles(const MaterialNode* root)
{
    using NodeSet = std::unordered_set<const MaterialNode*>;
    using FuncType = std::function<bool(const MaterialNode*, NodeSet&)>;

    FuncType detectCycle = [&](const MaterialNode* node, NodeSet& seenNodes)
    {
        auto [it, success] = seenNodes.emplace(node);
        if (!success) return true;

        for (auto child : node->getInputs())
        {
            if (detectCycle(child, seenNodes)) {
                return true;
            }
        }
        seenNodes.erase(it);

        return false;
    };

    NodeSet set;
    return detectCycle(root, set);
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
        ss << getResultIdentifier(node->getInputs().at(i));
        if (i < sig.inputs.size() - 1) {
            ss << ", ";
        }
    }
    ss << ")";

    return ss.str();
}

auto MaterialCompiler::getResultIdentifier(MaterialNode* node) -> std::string
{
    if (!identifiers.contains(node))
    {
        const auto valType = node->getFunction().getSignature().output.type;
        const auto id = makeIdentifier();

        std::stringstream ss;
        ss << valType.to_string() << " " << id << " = " << call(node) << ";";
        identifierDecls.emplace_back(ss.str());

        // This must be done after compiling the function's call code with `call`
        // because the arguments' identifiers have to be created first.
        identifiers.try_emplace(node, id);

        return id;
    }

    assert(!identifiers.at(node).empty());
    return identifiers.at(node);
}

auto MaterialCompiler::makeIdentifier() -> std::string
{
    return "_id_" + std::to_string(nextIdentifierId++);
}

} // namespace trc
