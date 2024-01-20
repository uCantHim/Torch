#pragma once

#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include "trc/core/PipelineLayoutTemplate.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/core/DescriptorProvider.h"

namespace trc
{
    class DescriptorRegistry;

    /**
     * @brief
     */
    class PipelineLayoutBuilder
    {
    public:
        using Self = PipelineLayoutBuilder;
        using Descriptor = PipelineLayoutTemplate::Descriptor;

        PipelineLayoutBuilder() = default;
        explicit
        PipelineLayoutBuilder(const PipelineLayoutTemplate& t);

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

        /**
         * @brief Define a descriptor set layout used by the pipeline
         *
         * Also supply a descriptor provider to be used to bind a descriptor set
         * whenever a pipeline using this layout is bound to a command buffer.
         */
        auto addStaticDescriptor(vk::DescriptorSetLayout descLayout,
                                 s_ptr<const DescriptorProviderInterface> provider) -> Self&;

        /**
         * @brief Define a descriptor set layout used by the pipeline
         */
        auto addDynamicDescriptor(vk::DescriptorSetLayout descLayout) -> Self&;

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
        auto build(const Device& device, const DescriptorRegistry& descRegistry)
            -> PipelineLayout;

        /**
         * @brief Build the layout and register it at a pipeline registry
         */
        auto registerLayout() const -> PipelineLayout::ID;

    private:
        using ProviderDefinition = std::tuple<vk::DescriptorSetLayout, s_ptr<const DescriptorProviderInterface>>;
        using DescriptorDefinition = std::variant<Descriptor, vk::DescriptorSetLayout, ProviderDefinition>;

        std::vector<DescriptorDefinition> descriptors;
        std::vector<PipelineLayoutTemplate::PushConstant> pushConstants;
    };

    auto buildPipelineLayout() -> PipelineLayoutBuilder;
    auto buildPipelineLayout(const PipelineLayoutTemplate& t) -> PipelineLayoutBuilder;
} // namespace trc
