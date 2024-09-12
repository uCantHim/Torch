#pragma once

#include "trc/base/Image.h"

#include "trc/core/Window.h"
#include "trc/core/DescriptorProvider.h"

namespace trc::rt
{
    struct RayBufferCreateInfo
    {
        uvec2 size;
        vk::ImageUsageFlags imageUsage;

        const DeviceMemoryAllocator& alloc{ DefaultDeviceMemoryAllocator{} };
    };

    class RayBuffer
    {
    public:
        enum Image
        {
            eReflections,

            NUM_IMAGES
        };

        RayBuffer(const Device& device, const RayBufferCreateInfo& info);

        auto getSize() const -> uvec2;
        auto getExtent() const -> vk::Extent2D;

        auto getImage(Image imageType) -> trc::Image&;
        auto getImage(Image imageType) const -> const trc::Image&;
        auto getImageView(Image imageType) const -> vk::ImageView;

        auto getImageDescriptor(Image imageType) const -> s_ptr<const DescriptorProvider>;
        auto getImageDescriptorSet(Image imageType) const -> vk::DescriptorSet;
        auto getImageDescriptorLayout() const -> vk::DescriptorSetLayout;

    private:
        void createDescriptors(const Device& device);

        const uvec2 size;

        std::vector<trc::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

        vk::UniqueDescriptorSetLayout layout;
        vk::UniqueDescriptorPool pool;
        std::vector<vk::UniqueDescriptorSet> sets;
        std::vector<s_ptr<DescriptorProvider>> singleImageProviders;
    };
} // namespace trc
