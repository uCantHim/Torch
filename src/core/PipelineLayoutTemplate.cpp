#include "PipelineLayoutTemplate.h"

#include "Pipeline.h"
#include "core/DescriptorProvider.h"



trc::PushConstantDefaultValue::PushConstantDefaultValue(std::vector<std::byte> rawData)
    :
    _setAsDefault([](const std::any& value, PipelineLayout& layout, vk::PushConstantRange range){
        auto data = std::any_cast<std::vector<std::byte>>(value);
        layout.addDefaultPushConstantValue(range.offset, std::move(data), range.stageFlags);
    }),
    value(std::move(rawData))
{
}

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
