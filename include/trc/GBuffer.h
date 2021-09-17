#pragma once

#include <vkb/Image.h>

#include "Types.h"

namespace trc
{
    struct GBufferCreateInfo
    {
        uvec2 size;
    };

    class GBuffer
    {
    public:
        enum Image : ui32
        {
            eNormals,
            eAlbedo,
            eMaterials,
            eDepth,

            NUM_IMAGES
        };

        GBuffer(const vkb::Device& device, const GBufferCreateInfo& size);

        auto getSize() const -> uvec2;
        auto getExtent() const -> vk::Extent2D;

        auto getImage(Image imageType) -> vkb::Image&;
        auto getImage(Image imageType) const -> const vkb::Image&;
        auto getImageView(Image imageType) const -> vk::ImageView;

    private:
        const uvec2 size;
        const vk::Extent2D extent;
        std::vector<vkb::Image> images;
        std::vector<vk::UniqueImageView> imageViews;
    };
} // namespace trc
