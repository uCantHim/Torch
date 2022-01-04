#pragma once

#include <vector>
#include <tuple>

#include <vkb/Device.h>

#include "../Types.h"

namespace trc
{
    class Instance;
    class DescriptorProviderInterface;

    /**
     * @brief A pipeline layout
     */
    class PipelineLayout
    {
    public:
        using ID = data::TypesafeID<PipelineLayout, ui32>;

        /**
         * The default constructor creates an uninitialized PipelineLayout.
         * In this case, the expression `*layout == VK_NULL_HANDLE`
         * evaluates to `true`.
         *
         * The PipelineLayout is implicitly convertible to `bool` to test
         * for this.
         */
        PipelineLayout() = default;

        explicit
        PipelineLayout(vk::UniquePipelineLayout layout);

        PipelineLayout(const vkb::Device& device,
                       const vk::ArrayProxy<const vk::DescriptorSetLayout>& descriptors,
                       const vk::ArrayProxy<const vk::PushConstantRange>& pushConstants);

        auto operator*() const noexcept -> vk::PipelineLayout;

        inline operator bool() const noexcept {
            return static_cast<bool>(layout);
        }

        /**
         * @brief Bind all static descriptor sets specified for the pipeline
         *
         * "Static" descriptor sets are the descriptor sets that are
         * pipeline-specific rather than draw-call-specific.
         */
        void bindStaticDescriptorSets(vk::CommandBuffer cmdBuf,
                                      vk::PipelineBindPoint bindPoint) const;

        /**
         * @brief Supply default values to specified push constant ranges
         */
        void bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const;

        void addStaticDescriptorSet(ui32 descriptorIndex,
                                    const DescriptorProviderInterface& provider) noexcept;
        template<typename T>
        void addDefaultPushConstantValue(ui32 offset, T value, vk::ShaderStageFlags stages);

    private:
        vk::UniquePipelineLayout layout;

        using PushConstantValue = std::tuple<ui32, vk::ShaderStageFlags, std::vector<uint8_t>>;
        std::vector<PushConstantValue> defaultPushConstants;
        std::vector<std::pair<ui32, const DescriptorProviderInterface*>> staticDescriptorSets;
    };

    /**
     * @brief Helper to create a pipeline layout
     *
     * @note It is possible to create PipelineLayouts with a constructor
     * that takes the exact same arguments. This function exists for
     * historical reasons.
     *
     * @param device
     * @param descriptorSetLayouts List of descriptor set layouts in the
     *                             pipeline
     * @param pushConstantRanges   List of push constant ranges in the
     *                             pipeline
     *
     * @return PipelineLayout
     */
    inline auto makePipelineLayout(
        const vkb::Device& device,
        const vk::ArrayProxy<const vk::DescriptorSetLayout>& descriptorSetLayouts,
        const vk::ArrayProxy<const vk::PushConstantRange>& pushConstantRanges)
        -> PipelineLayout
    {
        return PipelineLayout(device, descriptorSetLayouts, pushConstantRanges);
    }



    template<typename T>
    void PipelineLayout::addDefaultPushConstantValue(
        ui32 offset,
        T value,
        vk::ShaderStageFlags stages)
    {
        std::vector<uint8_t> defaultValue(sizeof(T));
        memcpy(defaultValue.data(), &value, sizeof(T));

        defaultPushConstants.emplace_back(offset, stages, std::move(defaultValue));
    }
} // namespace trc
