#pragma once

#include <vector>

#include <vkb/basics/Device.h>

#include "Types.h"

namespace trc
{
    class DescriptorSetLayoutBuilder
    {
    public:
        using Self = DescriptorSetLayoutBuilder;

        inline auto setFlags(vk::DescriptorSetLayoutCreateFlags flags) -> Self&
        {
            this->flags = flags;
            return *this;
        }

        inline auto addFlag(vk::DescriptorSetLayoutCreateFlags flags) -> Self&
        {
            this->flags |= flags;
            return *this;
        }

        inline auto addBinding(vk::DescriptorType type, ui32 count, vk::ShaderStageFlags stages)
            -> Self&
        {
            bindings.emplace_back(bindings.size(), type, count, stages);
            return *this;
        }

        inline auto build(const vkb::Device& device) -> vk::DescriptorSetLayout
        {
            return device->createDescriptorSetLayout({ flags, bindings });
        }

        inline auto buildUnique(const vkb::Device& device) -> vk::UniqueDescriptorSetLayout
        {
            return device->createDescriptorSetLayoutUnique({ flags, bindings });
        }

    private:
        vk::DescriptorSetLayoutCreateFlags flags;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };

    inline auto buildDescriptorSetLayout() -> DescriptorSetLayoutBuilder
    {
        return DescriptorSetLayoutBuilder{};
    }
} // namespace trc
