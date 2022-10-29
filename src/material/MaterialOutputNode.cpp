#include "trc/material/MaterialOutputNode.h"

#include "trc/material/MaterialNode.h"



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

void MaterialOutputNode::linkOutput(ParameterID param, OutputID output, std::string accessor)
{
    paramOutputLinks.push_back({ param, output, accessor });
}

void MaterialOutputNode::setParameter(ParameterID param, MaterialNode* value)
{
    const auto type = paramTypes.at(param.index);
    if (type.channels != value->getFunction().getSignature().output.type.channels)
    {
        throw std::invalid_argument(
            "[In MaterialResultNode::setParameter]: Parameter " + std::to_string(param.index)
            + " has a channel count of " + std::to_string(type.channels) + ", which does not match"
            " the channel count of the provided value.");
    }

    paramNodes.at(param.index) = value;
}

auto MaterialOutputNode::getParameter(ParameterID param) const -> MaterialNode*
{
    return paramNodes.at(param.index);
}

auto MaterialOutputNode::getParameters() const -> const std::vector<MaterialNode*>&
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

} // namespace trc
