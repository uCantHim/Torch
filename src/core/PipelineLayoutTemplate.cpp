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



auto trc::makePipelineLayout(
    const vkb::Device& device,
    const PipelineLayoutTemplate& _template,
    const RenderConfig& renderConfig)
    -> PipelineLayout
{
    std::vector<vk::DescriptorSetLayout> descLayouts;
    for (const auto& desc : _template.getDescriptors())
    {
        descLayouts.emplace_back(renderConfig.getDescriptorLayout(desc.name));
    }

    std::vector<vk::PushConstantRange> pushConstantRanges;
    for (const auto& pc : _template.getPushConstants())
    {
        pushConstantRanges.emplace_back(pc.range);
    }

    vk::PipelineLayoutCreateInfo createInfo{ {}, descLayouts, pushConstantRanges };
    PipelineLayout layout{ device->createPipelineLayoutUnique(createInfo) };

    // Add static descriptors and default push constant values to the layout
    for (ui32 i = 0; auto& desc : _template.getDescriptors())
    {
        if (desc.isStatic) {
            layout.addStaticDescriptorSet(i, renderConfig.getDescriptorID(desc.name));
        }
        ++i;
    }

    for (const auto& push : _template.getPushConstants())
    {
        if (push.defaultValue.has_value()) {
            push.defaultValue.value().setAsDefault(layout, push.range);
        }
    }

    return layout;
}
