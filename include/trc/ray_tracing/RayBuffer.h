#pragma once

#include <vkb/Image.h>

#include "core/Window.h"
#include "core/DescriptorProvider.h"

namespace trc::rt
{
    struct RayBufferCreateInfo
    {
        uvec2 size;
        vk::ImageUsageFlags imageUsage;

        const vkb::DeviceMemoryAllocator& alloc{ vkb::DefaultDeviceMemoryAllocator{} };
    };

    class RayBuffer
    {
    public:
        enum Image
        {
            eReflections,

            NUM_IMAGES
        };

        RayBuffer(const vkb::Device& device, const RayBufferCreateInfo& info);

        auto getSize() const -> uvec2;
        auto getExtent() const -> vk::Extent2D;

        auto getImage(Image imageType) -> vkb::Image&;
        auto getImage(Image imageType) const -> const vkb::Image&;
        auto getImageView(Image imageType) const -> vk::ImageView;

        auto getImageDescriptor(Image imageType) const -> const DescriptorProvider&;
        auto getImageDescriptorLayout() const -> vk::DescriptorSetLayout;

    private:
        void createDescriptors(const vkb::Device& device);

        const uvec2 size;

        std::vector<vkb::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

        vk::UniqueDescriptorSetLayout layout;
        vk::UniqueDescriptorPool pool;
        std::vector<vk::UniqueDescriptorSet> sets;
        std::vector<u_ptr<DescriptorProvider>> singleImageProviders;
    };
} // namespace trc
