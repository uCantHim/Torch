#pragma once

#include "core/DescriptorProvider.h"

namespace trc
{
    /**
     * @brief Wraps exchangeable descriptor providers into an object
     *
     * This is useful in cases where a provider object with a reliable
     * lifetime is required but in reality the provider can change
     * unpredictably.
     */
    class DescriptorProviderWrapper : public DescriptorProviderInterface
    {
    public:
        DescriptorProviderWrapper() = default;
        explicit DescriptorProviderWrapper(vk::DescriptorSetLayout staticLayout);

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

        /**
         * @brief Change the contained provider
         *
         * Also changes the descriptor set layout to that of the new
         * provider.
         *
         * @param const DescriptorProviderInterface& wrapped
         */
        void setWrappedProvider(const DescriptorProviderInterface& wrapped) noexcept;

        /**
         * @brief Change the contained provider
         *
         * Also changes the descriptor set layout to that of the new
         * provider.
         *
         * @param const DescriptorProviderInterface& wrapped
         */
        auto operator=(const DescriptorProviderInterface& wrapped) noexcept
            -> DescriptorProviderWrapper&;

        /**
         * @brief Change the provided descriptor set layout
         */
        void setDescLayout(vk::DescriptorSetLayout layout);

    private:
        // The actual descriptor provider
        const DescriptorProviderInterface* provider{ nullptr };

        // Use a static descriptor set layout. All exchangable
        // providers must have one.
        vk::DescriptorSetLayout descLayout;
    };
}
