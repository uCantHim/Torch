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
     * @brief
     */
    class PipelineLayout
    {
    public:
        using ID = data::TypesafeID<PipelineLayout, ui32>;

        PipelineLayout() = default;
        explicit
        PipelineLayout(vk::UniquePipelineLayout layout);
        PipelineLayout(const vkb::Device& device,
                       const std::vector<vk::DescriptorSetLayout>& descriptors,
                       const std::vector<vk::PushConstantRange>& pushConstants);

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



    template<typename T>
    void PipelineLayout::addDefaultPushConstantValue(
        ui32 offset,
        T value,
        vk::ShaderStageFlags stages)
    {
        std::vector<uint8_t> defaultValue;
        defaultValue.resize(sizeof(T));
        memcpy(defaultValue.data(), &value, sizeof(T));

        defaultPushConstants.emplace_back(offset, stages, std::move(defaultValue));
    }
} // namespace trc
