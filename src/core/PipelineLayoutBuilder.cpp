#include "PipelineLayoutBuilder.h"



trc::PipelineLayoutBuilder::PipelineLayoutBuilder(const PipelineLayoutTemplate& t)
    :
    pushConstants(t.getPushConstants())
{
    for (const auto& d : t.getDescriptors()) {
        descriptors.emplace_back(d);
    }
}

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

auto trc::PipelineLayoutBuilder::build(const vkb::Device& device, RenderConfig& renderConfig)
    -> PipelineLayout
{
    // Collect descriptors
    std::vector<vk::DescriptorSetLayout> descLayouts;
    std::vector<std::pair<const DescriptorProviderInterface*, bool>> providers;
    auto addProvider = [&](const DescriptorProviderInterface* p, bool b)
    {
        providers.emplace_back(p, b);
        descLayouts.emplace_back(p->getDescriptorSetLayout());
    };

    for (const auto& def : descriptors)
    {
        if (std::holds_alternative<ProviderDefinition>(def))
        {
            auto [p, isStatic] = std::get<ProviderDefinition>(def);
            addProvider(p, isStatic);
        }
        else {
            const auto& descName = std::get<Descriptor>(def);
            const auto id = renderConfig.getDescriptorID(descName.name);
            addProvider(&renderConfig.getDescriptor(id), descName.isStatic);
        }
    }

    // Collect push constant ranges
    std::vector<vk::PushConstantRange> pcRanges;
    for (const auto& pc : pushConstants) {
        pcRanges.emplace_back(pc.range);
    }

    auto layout = makePipelineLayout(device, std::move(descLayouts), std::move(pcRanges));

    // Set static descriptors and default push constant values
    for (ui32 i = 0; const auto& [p, isStatic] : providers)
    {
        if (isStatic) {
            layout.addStaticDescriptorSet(i, *p);
        }
        ++i;
    }
    for (const auto& pc : pushConstants)
    {
        if (pc.defaultValue.has_value()) {
            pc.defaultValue.value().setAsDefault(layout, pc.range);
        }
    }

    return layout;
}

auto trc::PipelineLayoutBuilder::registerLayout() const -> PipelineLayout::ID
{
    return PipelineRegistry::registerPipelineLayout(build());
}



auto trc::buildPipelineLayout() -> PipelineLayoutBuilder
{
    return PipelineLayoutBuilder{};
}

auto trc::buildPipelineLayout(const PipelineLayoutTemplate& t) -> PipelineLayoutBuilder
{
    return PipelineLayoutBuilder{ t };
}
