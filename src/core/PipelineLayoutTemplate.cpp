#include "PipelineLayoutTemplate.h"

#include "Pipeline.h"
#include "core/DescriptorProvider.h"



void trc::PushConstantDefaultValue::setAsDefault(
    PipelineLayout& layout,
    vk::PushConstantRange range) const
{
    _setAsDefault(value, layout, range);
}



trc::PipelineLayoutTemplate::PipelineLayoutTemplate(
    const std::vector<Descriptor>& descriptors,
    const std::vector<PushConstant>& pushConstants)
    :
    descriptors(descriptors),
    pushConstants(pushConstants)
{
}

auto trc::PipelineLayoutTemplate::getDescriptors() const -> const std::vector<Descriptor>&
{
    return descriptors;
}

auto trc::PipelineLayoutTemplate::getPushConstants() const -> const std::vector<PushConstant>&
{
    return pushConstants;
}
