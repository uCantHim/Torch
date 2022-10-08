#pragma once

#include <vector>

#include "trc/base/Device.h"

#include "trc/Types.h"

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

        auto build(const Device& device) -> vk::UniqueDescriptorSetLayout;

    private:
        vk::DescriptorSetLayoutCreateFlags layoutFlags;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<vk::DescriptorBindingFlags> bindingFlags;
    };

    inline auto buildDescriptorSetLayout() -> DescriptorSetLayoutBuilder
    {
        return DescriptorSetLayoutBuilder{};
    }
} // namespace trc
