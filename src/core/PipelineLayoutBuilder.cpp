#include "trc/core/PipelineLayoutBuilder.h"

#include <trc_util/TypeUtils.h>

#include "trc/core/DescriptorRegistry.h"



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

auto trc::PipelineLayoutBuilder::addStaticDescriptor(
    vk::DescriptorSetLayout descLayout,
    s_ptr<const DescriptorProviderInterface> provider) -> Self&
{
    descriptors.emplace_back(ProviderDefinition{ descLayout, std::move(provider) });
    return *this;
}

auto trc::PipelineLayoutBuilder::addDynamicDescriptor(vk::DescriptorSetLayout descLayout) -> Self&
{
    descriptors.emplace_back(descLayout);
    return *this;
}

auto trc::PipelineLayoutBuilder::addPushConstantRange(
    vk::PushConstantRange range,
    std::optional<PushConstantDefaultValue> defaultValue
    ) -> Self&
{
    pushConstants.push_back({ range, std::move(defaultValue) });
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

    return { descNames, pushConstants };
}

auto trc::PipelineLayoutBuilder::build(
    const Device& device,
    const DescriptorRegistry& descRegistry)
    -> PipelineLayout
{
    // Collect descriptor set layouts
    std::vector<vk::DescriptorSetLayout> descLayouts;
    for (const auto& def : descriptors)
    {
        descLayouts.emplace_back(
            std::visit(util::VariantVisitor{
                [&](const Descriptor& desc) { return descRegistry.getDescriptorLayout(desc.name); },
                [&](const vk::DescriptorSetLayout& layout) { return layout; },
                [&](const ProviderDefinition& def) { return std::get<0>(def); },
            }, def)
        );
    }

    // Collect push constant ranges
    std::vector<vk::PushConstantRange> pcRanges;
    for (const auto& pc : pushConstants) {
        pcRanges.emplace_back(pc.range);
    }

    auto layout = makePipelineLayout(device, descLayouts, pcRanges);

    // Set statically-bound descriptors
    for (ui32 i = 0; const auto& def : descriptors)
    {
        std::visit(util::VariantVisitor{
            [&](const Descriptor& desc) {
                layout.addStaticDescriptorSet(i, descRegistry.getDescriptorID(desc.name));
            },
            [&](const vk::DescriptorSetLayout& /*layout*/) { /* Is a dynamically-bound set */ },
            [&](const ProviderDefinition& def) {
                const auto& [_, provider] = def;
                layout.addStaticDescriptorSet(i, provider);
            },
        }, def);
        ++i;
    }

    // Set default push constant values
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
