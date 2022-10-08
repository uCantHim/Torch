#pragma once

#include <vector>
#include <tuple>

#include "trc/base/Device.h"

#include "trc/Types.h"
#include "trc/core/DescriptorRegistry.h"

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

        PipelineLayout(const Device& device,
                       const vk::ArrayProxy<const vk::DescriptorSetLayout>& descriptors,
                       const vk::ArrayProxy<const vk::PushConstantRange>& pushConstants);

        auto operator*() const noexcept -> vk::PipelineLayout;

        inline operator bool() const noexcept {
            return static_cast<bool>(layout);
        }

        /**
         * @brief Bind all static descriptor sets specified for the pipeline
         *
         * Only bind static descriptor sets that are defined through a
         * DescriptorProvider. Don't bind the ones defined through a
         * DescriptorID.
         *
         * "Static" descriptor sets are the descriptor sets that are
         * pipeline-specific rather than draw-call-specific.
         */
        void bindStaticDescriptorSets(vk::CommandBuffer cmdBuf,
                                      vk::PipelineBindPoint bindPoint) const;

        /**
         * @brief Bind all static descriptor sets specified for the pipeline
         *
         * Use a descriptor registry to query and bind descriptors that are
         * dynamically defined by a DescriptorID.
         *
         * "Static" descriptor sets are the descriptor sets that are
         * pipeline-specific rather than draw-call-specific.
         */
        void bindStaticDescriptorSets(vk::CommandBuffer cmdBuf,
                                      vk::PipelineBindPoint bindPoint,
                                      const DescriptorRegistry& reg) const;

        /**
         * @brief Supply default values to specified push constant ranges
         */
        void bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const;

        /**
         * @brief Define a static descriptor for the pipeline layout
         *
         * This function defines a descriptor provider that will be used
         * to bind a descriptor set whenever a pipeline with this layout
         * is bound to a command buffer.
         */
        void addStaticDescriptorSet(ui32 descriptorIndex,
                                    const DescriptorProviderInterface& provider) noexcept;

        /**
         * @brief Define a static descriptor for the pipeline layout
         *
         * This function defines a descriptor ID that is used to
         * dynamically query and bind a descriptor set from a
         * DescriptorRegistry whenever a pipeline with this layout is bound
         * to a command buffer.
         */
        void addStaticDescriptorSet(ui32 descriptorIndex, DescriptorID id) noexcept;

        /**
         * @brief Set default data for a push constant range
         *
         * @param ui32 offset
         * @param std::vector<std::byte> data Raw data
         * @param vk::ShaderStageFlags stages All stages for which the data
         *                                    should be set as default.
         */
        void addDefaultPushConstantValue(ui32 offset,
                                         std::vector<std::byte> data,
                                         vk::ShaderStageFlags stages);

        /**
         * @brief Set default data for a push constant range
         *
         * Set default data for the range `[offset, offset + sizeof(T)]`.
         *
         * @param ui32 offset
         * @param T value
         * @param vk::ShaderStageFlags stages All stages for which the data
         *                                    should be set as default.
         */
        template<typename T>
        void addDefaultPushConstantValue(ui32 offset, const T& value, vk::ShaderStageFlags stages);

    private:
        vk::UniquePipelineLayout layout;

        using PushConstantValue = std::tuple<ui32, vk::ShaderStageFlags, std::vector<std::byte>>;
        std::vector<PushConstantValue> defaultPushConstants;
        std::vector<std::pair<ui32, DescriptorID>> dynamicDescriptorSets;
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
        const Device& device,
        const vk::ArrayProxy<const vk::DescriptorSetLayout>& descriptorSetLayouts,
        const vk::ArrayProxy<const vk::PushConstantRange>& pushConstantRanges)
        -> PipelineLayout
    {
        return PipelineLayout(device, descriptorSetLayouts, pushConstantRanges);
    }



    inline void PipelineLayout::addDefaultPushConstantValue(
        ui32 offset,
        std::vector<std::byte> data,
        vk::ShaderStageFlags stages)
    {
        if (!data.empty()) {
            defaultPushConstants.emplace_back(offset, stages, std::move(data));
        }
    }

    template<typename T>
    inline void PipelineLayout::addDefaultPushConstantValue(
        ui32 offset,
        const T& value,
        vk::ShaderStageFlags stages)
    {
        std::vector<std::byte> defaultValue(sizeof(T));
        memcpy(defaultValue.data(), &value, sizeof(T));
        defaultPushConstants.emplace_back(offset, stages, std::move(defaultValue));
    }
} // namespace trc
