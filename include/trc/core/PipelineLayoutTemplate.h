#pragma once

#include <concepts>
#include <vector>
#include <any>

#include "../Types.h"
#include "RenderConfiguration.h"

namespace trc
{
    class PushConstantDefaultValue
    {
    public:
        PushConstantDefaultValue(const PushConstantDefaultValue&) = default;
        PushConstantDefaultValue(PushConstantDefaultValue&&) noexcept = default;
        auto operator=(const PushConstantDefaultValue&) -> PushConstantDefaultValue& = default;
        auto operator=(PushConstantDefaultValue&&) noexcept -> PushConstantDefaultValue& = default;
        ~PushConstantDefaultValue() = default;

        explicit PushConstantDefaultValue(std::vector<std::byte> rawData);
        template<typename T>
        PushConstantDefaultValue(T&& value);

        void setAsDefault(PipelineLayout& layout, vk::PushConstantRange range) const;

    private:
        void(*_setAsDefault)(const std::any&, PipelineLayout&, vk::PushConstantRange);
        std::any value;
    };

    /**
     * @brief High-level description data that defines a pipeline layout
     */
    class PipelineLayoutTemplate
    {
    public:
        struct Descriptor
        {
            DescriptorName name;
            bool isStatic{ true };
        };

        struct PushConstant
        {
            vk::PushConstantRange range;
            std::optional<PushConstantDefaultValue> defaultValue;
        };

        /**
         * @brief
         */
        PipelineLayoutTemplate() = default;
        PipelineLayoutTemplate(const std::vector<Descriptor>& descriptors,
                               const std::vector<PushConstant>& pushConstants);

        auto getDescriptors() const -> const std::vector<Descriptor>&;
        auto getPushConstants() const -> const std::vector<PushConstant>&;

    private:
        std::vector<Descriptor> descriptors;
        std::vector<PushConstant> pushConstants;
    };

    /**
     * @brief Create a pipeline layout from a template
     */
    template<RenderConfigType T>
    auto makePipelineLayout(const vkb::Device& device,
                            const PipelineLayoutTemplate& _template,
                            const T& renderConfig
        ) -> PipelineLayout;



    ////////////////////////////////
    //  Template Implementations  //
    ////////////////////////////////

    template<typename T>
    PushConstantDefaultValue::PushConstantDefaultValue(T&& value)
        :
        _setAsDefault(
            [](const std::any& value, PipelineLayout& layout, vk::PushConstantRange range)
            {
                assert(sizeof(T) == range.size);
                layout.addDefaultPushConstantValue(
                    range.offset,
                    std::any_cast<T>(value),
                    range.stageFlags
                );
            }
        ),
        value(std::forward<T>(value))
    {}

    template<RenderConfigType T>
    auto makePipelineLayout(
        const vkb::Device& device,
        const PipelineLayoutTemplate& _template,
        const T& renderConfig)
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
} // namespace trc
