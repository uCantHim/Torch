#pragma once

#include <vector>
#include <variant>

#include "trc/base/Device.h"

#include "trc/VulkanInclude.h"
#include "trc/Types.h"

namespace trc
{
    struct DescriptorLayoutBindingInfo
    {
        ui32 binding;
        vk::DescriptorType type;
        ui32 numDescriptors;
        vk::ShaderStageFlags stages;

        vk::DescriptorSetLayoutCreateFlags layoutFlags{};
        vk::DescriptorBindingFlags bindingFlags{};  // optional, pNext
    };

    inline auto makeDescriptorPool(const Device& device,
                                   vk::DescriptorPoolCreateFlags createFlags,
                                   ui32 maxSets,
                                   const std::vector<DescriptorLayoutBindingInfo>& info)
        -> vk::UniqueDescriptorPool
    {
        vk::DescriptorSetLayoutCreateFlags layoutFlags;
        std::vector<vk::DescriptorPoolSize> poolSizes;
        for (const auto& binding : info)
        {
            layoutFlags |= binding.layoutFlags;
            poolSizes.emplace_back(binding.type, binding.numDescriptors);
        }

        if (layoutFlags & vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool) {
            createFlags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        }

        return device->createDescriptorPoolUnique({ createFlags, maxSets, poolSizes });
    }

    inline auto makeDescriptorSetLayout(const Device& device,
                                        const std::vector<DescriptorLayoutBindingInfo>& info)
        -> vk::UniqueDescriptorSetLayout
    {
        const size_t numBindings = info.size();

        vk::DescriptorSetLayoutCreateFlags createFlags;
        std::vector<vk::DescriptorBindingFlags> bindingFlags(numBindings);
        std::vector<vk::DescriptorSetLayoutBinding> bindings(numBindings);
        for (const auto& binding : info)
        {
            createFlags |= binding.layoutFlags;
            bindings.at(binding.binding) = vk::DescriptorSetLayoutBinding(
                binding.binding,
                binding.type,
                binding.numDescriptors,
                binding.stages
            );
            bindingFlags.at(binding.binding) = binding.bindingFlags;
        }

        vk::StructureChain chain{
            vk::DescriptorSetLayoutCreateInfo(createFlags, bindings),
            vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
        };

        return device->createDescriptorSetLayoutUnique(chain.get());
    }
} // namespace trc
