#include "PipelineLayoutBuilder.h"



auto trc::PipelineLayoutBuilder::addDescriptor(
    const DescriptorName& name,
    bool hasStaticSet
    ) -> Self&
{
    descriptors.emplace_back(std::move(name), hasStaticSet);
    return *this;
}

auto trc::PipelineLayoutBuilder::addDescriptorIf(
    const bool condition,
    const DescriptorName& name,
    bool hasStaticSet
    ) -> Self&
{
    if (condition) addDescriptor(name, hasStaticSet);
    return *this;
}

auto trc::PipelineLayoutBuilder::addPushConstantRange(
    vk::PushConstantRange range,
    std::optional<PushConstantDefaultValue> defaultValue
    ) -> Self&
{
    pushConstants.emplace_back(range, std::move(defaultValue));
    return *this;
}

auto trc::PipelineLayoutBuilder::addPushConstantRangeIf(
    const bool condition,
    vk::PushConstantRange range,
    std::optional<PushConstantDefaultValue> defaultValue
    ) -> Self&
{
    if (condition) addPushConstantRange(range, std::move(defaultValue));
    return *this;
}

auto trc::PipelineLayoutBuilder::build() const -> PipelineLayoutTemplate
{
    return { descriptors, pushConstants };
}



auto trc::buildPipelineLayout() -> PipelineLayoutBuilder
{
    return PipelineLayoutBuilder{};
}
