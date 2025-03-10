#pragma once

#include <concepts>
#include <vector>
#include <any>

#include "trc/Types.h"
#include "trc/core/DescriptorRegistry.h"
#include "trc/core/PipelineLayout.h"

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

        PushConstantDefaultValue(std::vector<std::byte> rawData);
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
    auto makePipelineLayout(const Device& device,
                            const PipelineLayoutTemplate& _template,
                            const DescriptorRegistry& descRegistry
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
} // namespace trc
