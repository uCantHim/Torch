#pragma once

#include <vector>
#include <optional>

#include "PipelineLayoutTemplate.h"
#include "PipelineRegistry.h"
#include "DescriptorProvider.h"

namespace trc
{
    /**
     * @brief
     */
    class PipelineLayoutBuilder
    {
    public:
        using Self = PipelineLayoutBuilder;
        using Descriptor = PipelineLayoutTemplate::Descriptor;

        PipelineLayoutBuilder() = default;

        /**
         * Idea:
         *
         * I could make another wrapper around the descriptor name. It is a
         * very general adapter that can provide descriptor set layouts for
         * dynamic descriptors and layouts + sets for static descriptors.
         *
         * struct Descriptor
         * {
         *     Descriptor(vk::DescriptorSetLayout);
         *     Descriptor(DescriptorProvider);
         *     Descriptor(DescriptorName);
         *
         *     // Or, probably too general and fancy:
         *     Descriptor(std::function<vk::DescriptorSetLayout(const RenderConfig&)>);
         *
         *     auto getLayout(const DescriptorRegistry&) -> vk::DescriptorSetLayout;
         *
         *     // Only for static descriptors:
         *     auto getSetProvider(const DescriptorRegistry&) -> DescriptorProviderInterface&;
         *
         * private:
         *     bool static;  // <-- Deduced from constructor argument
         *     InternalVirtualThing internal;  // <-- Maybe a variant?
         * };
         *
         * This adapter is resolved at pipeline layout creation. Zero
         * consistent runtime cost. Initialization cost of an absolutely
         * negligible virtual call.
         */

        /**
         * Descriptors are placed at descriptor set indices in the order in
         * which they were added to the layout.
         */
        auto addDescriptor(const DescriptorName& name, bool hasStaticSet = true) -> Self&;

        /**
         * @brief Conditionally add a descriptor
         *
         * The builder's flowing interface doesn't have to be interrupted
         * if you want to add a descriptor only if a certain condition is
         * met; for example if a device feature is present.
         */
        auto addDescriptorIf(bool condition,
                             const DescriptorName& name,
                             bool hasStaticSet = true
                             ) -> Self&;

        auto addDescriptor(const DescriptorProviderInterface& provider, bool hasStaticSet) -> Self&;

        /**
         * Only call this function once per stage in vk::PipelineStageFlags.
         * The Vulkan specification requires this.
         */
        auto addPushConstantRange(vk::PushConstantRange range,
                                  std::optional<PushConstantDefaultValue> defaultValue = {}
                                  ) -> Self&;

        /**
         * Only call this function once per stage in vk::PipelineStageFlags.
         * The Vulkan specification requires this.
         */
        auto addPushConstantRangeIf(bool condition,
                                    vk::PushConstantRange range,
                                    std::optional<PushConstantDefaultValue> defaultValue = {}
                                    ) -> Self&;

        /**
         * @brief Build a pipeline layout template
         */
        auto build() const -> PipelineLayoutTemplate;

        /**
         * @brief Build a pipeline layout
         */
        template<RenderConfigType T>
        auto build(const vkb::Device& device, T& renderConfig) -> PipelineLayout;

        /**
         * @brief Build the layout and register it at a pipeline registry
         */
        template<RenderConfigType T>
        auto registerLayout() const -> PipelineLayout::ID;

    private:
        using ProviderDefinition = std::pair<const DescriptorProviderInterface*, bool>;
        using DescriptorDefinition = std::variant<Descriptor, ProviderDefinition>;

        std::vector<DescriptorDefinition> descriptors;
        std::vector<PipelineLayoutTemplate::PushConstant> pushConstants;
    };

    auto buildPipelineLayout() -> PipelineLayoutBuilder;



    template<RenderConfigType T>
    auto PipelineLayoutBuilder::build(const vkb::Device& device, T& renderConfig) -> PipelineLayout
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

    template<RenderConfigType T>
    auto PipelineLayoutBuilder::registerLayout() const -> PipelineLayout::ID
    {
        return PipelineRegistry<T>::registerPipelineLayout(build());
    }
} // namespace trc
