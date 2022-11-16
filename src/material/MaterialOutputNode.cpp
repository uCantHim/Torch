#include "trc/material/MaterialOutputNode.h"



namespace trc
{

auto MaterialOutputNode::addParameter(BasicType type) -> ParameterID
{
    paramNodes.emplace_back(nullptr);
    paramTypes.emplace_back(type);
    return { static_cast<ui32>(paramNodes.size() - 1) };
}

auto MaterialOutputNode::addOutput(ui32 location, BasicType type) -> OutputID
{
    outputLocations.push_back({ location, type });
    return { static_cast<ui32>(outputLocations.size() - 1) };
}

auto MaterialOutputNode::addBuiltinOutput(const std::string& outputName) -> BuiltinOutputID
{
    builtinOutputNames.emplace_back(outputName);
    return { static_cast<ui32>(builtinOutputNames.size() - 1) };
}

void MaterialOutputNode::linkOutput(ParameterID param, OutputID output, std::string accessor)
{
    paramOutputLinks.push_back({ param, output, accessor });
}

void MaterialOutputNode::linkOutput(ParameterID param, BuiltinOutputID output)
{
    assert(builtinOutputNames.size() > output.index);
    builtinOutputLinks.try_emplace(builtinOutputNames.at(output.index), param);
}

void MaterialOutputNode::setParameter(ParameterID param, code::Value value)
{
    // const auto type = paramTypes.at(param.index);
    // if (type.channels != value->getFunction().getType().output.type.channels)
    // {
    //     throw std::invalid_argument(
    //         "[In MaterialResultNode::setParameter]: Parameter " + std::to_string(param.index)
    //         + " has a channel count of " + std::to_string(type.channels) + ", which does not match"
    //         " the channel count of the provided value.");
    // }

    paramNodes.at(param.index) = value;
}

auto MaterialOutputNode::getParameter(ParameterID param) const -> code::Value
{
    return paramNodes.at(param.index);
}

auto MaterialOutputNode::getParameters() const -> const std::vector<code::Value>&
{
    return paramNodes;
}

auto MaterialOutputNode::getOutputLinks() const -> const std::vector<ParameterOutputLink>&
{
    return paramOutputLinks;
}

auto MaterialOutputNode::getOutput(OutputID output) const -> const OutputLocation&
{
    return outputLocations.at(output.index);
}

auto MaterialOutputNode::getOutputs() const -> const std::vector<OutputLocation>&
{
    return outputLocations;
}

auto MaterialOutputNode::getBuiltinOutputs() const
    -> const std::unordered_map<std::string, ParameterID>&
{
    return builtinOutputLinks;
}

} // namespace trc
