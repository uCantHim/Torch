#include "PipelineLayoutBuilder.h"



auto trc::PipelineLayoutBuilder::addDescriptor(
    const DescriptorName& name,
    bool hasStaticSet
    ) -> Self&
{
    descriptors.emplace_back(Descriptor{ name, hasStaticSet });
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

auto trc::PipelineLayoutBuilder::addDescriptor(
    const DescriptorProviderInterface& provider,
    bool hasStaticSet
    ) -> Self&
{
    descriptors.emplace_back(ProviderDefinition{ &provider, hasStaticSet });
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
    std::vector<Descriptor> descNames;
    for (const auto& def : descriptors)
    {
        if (std::holds_alternative<ProviderDefinition>(def))
        {
            throw Exception("[In PipelineLayoutBuilder::build]: Contains a descriptor definition"
                            " that is not a descriptor name; cannot build a template from this.");
        }
        descNames.emplace_back(std::get<Descriptor>(def));
    }

    return { std::move(descNames), pushConstants };
}



auto trc::buildPipelineLayout() -> PipelineLayoutBuilder
{
    return PipelineLayoutBuilder{};
}
