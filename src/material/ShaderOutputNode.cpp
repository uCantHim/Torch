#include "trc/material/ShaderOutputNode.h"



namespace trc
{

auto ShaderOutputNode::addParameter(BasicType type) -> ParameterID
{
    paramNodes.emplace_back(nullptr);
    paramTypes.emplace_back(type);
    return { static_cast<ui32>(paramNodes.size() - 1) };
}

auto ShaderOutputNode::addOutput(ui32 location, BasicType type) -> OutputID
{
    outputLocations.push_back({ location, type });
    return { static_cast<ui32>(outputLocations.size() - 1) };
}

void ShaderOutputNode::linkOutput(ParameterID param, OutputID output, std::string accessor)
{
    paramOutputLinks.push_back({ param, output, accessor });
}

void ShaderOutputNode::setParameter(ParameterID param, code::Value value)
{
    paramNodes.at(param.index) = value;
}

auto ShaderOutputNode::getParameter(ParameterID param) const -> code::Value
{
    return paramNodes.at(param.index);
}

auto ShaderOutputNode::getParameters() const -> const std::vector<code::Value>&
{
    return paramNodes;
}

auto ShaderOutputNode::getOutputLinks() const -> const std::vector<ParameterOutputLink>&
{
    return paramOutputLinks;
}

auto ShaderOutputNode::getOutput(OutputID output) const -> const OutputLocation&
{
    return outputLocations.at(output.index);
}

auto ShaderOutputNode::getOutputs() const -> const std::vector<OutputLocation>&
{
    return outputLocations;
}

} // namespace trc
