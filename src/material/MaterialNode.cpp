#include "trc/material/MaterialNode.h"



namespace trc
{

MaterialNode::MaterialNode(u_ptr<MaterialFunction> func)
    :
    inputs(func->getSignature().inputs.size(), nullptr),
    function(std::move(func))
{
}

void MaterialNode::setInput(ui32 input, MaterialNode* node)
{
    const ui32 inputChannels = node->getFunction().getSignature().output.type.channels;
    assert(function->getSignature().inputs.size() > input);
    assert(function->getSignature().inputs.at(input).type.channels == inputChannels);
    assert(inputs.size() > input);

    inputs.at(input) = node;
}

auto MaterialNode::getInputs() const -> const std::vector<MaterialNode*>&
{
    return inputs;
}

auto MaterialNode::getOutputChannelCount() const -> ui32
{
    return function->getSignature().output.type.channels;
}

auto MaterialNode::getFunction() -> MaterialFunction&
{
    return *function;
}

} // namespace trc
