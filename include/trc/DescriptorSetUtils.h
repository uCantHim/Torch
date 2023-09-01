#pragma once

#include <vector>

#include "trc/Types.h"
#include "trc/base/Device.h"

namespace trc
{
    class DescriptorSetLayoutBuilder
    {
    public:
        using Self = DescriptorSetLayoutBuilder;

        auto addFlag(vk::DescriptorSetLayoutCreateFlagBits flags) -> Self&;
        auto addBinding(vk::DescriptorType type,
                        ui32 count,
                        vk::ShaderStageFlags stages,
                        vk::DescriptorBindingFlags flags = {})
            -> Self&;

        auto getBindingCount() const -> size_t;

        /**
         * @brief Build a descriptor set layout
         *
         * Builds a descriptor set layout with the previously specified bindings
         * and flags.
         *
         * @param const Device& device
         */
        auto build(const Device& device) -> vk::UniqueDescriptorSetLayout;

        /**
         * @brief Build a descriptor pool with capacity to instantiate the layout
         *
         * @param const Device& device
         * @param ui32 maxSets The number of descriptor sets of the currently
         *                     built layout for which the pool shall have
         *                     enough capacity.
         * @param vk::DescriptorPoolCreateFlags Additional create flags for the
         *        descriptor pool. The flag bits
         *        `vk::DescriptorPoolCreateFlagBits::eHostOnlyEXT` and
         *        `vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind` are added
         *        automatically if the corresponding descriptor layout flag bits
         *        are specified via `DescriptorSetLayoutBuilder::addFlag`.
         */
        auto buildPool(const Device& device, ui32 maxSets, vk::DescriptorPoolCreateFlags flags = {})
            -> vk::UniqueDescriptorPool;

    private:
        vk::DescriptorSetLayoutCreateFlags layoutFlags;
        vk::DescriptorPoolCreateFlags poolFlags;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<vk::DescriptorBindingFlags> bindingFlags;
    };

    inline auto buildDescriptorSetLayout() -> DescriptorSetLayoutBuilder
    {
        return DescriptorSetLayoutBuilder{};
    }
} // namespace trc
