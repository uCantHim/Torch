#pragma once

#include <vkb/Swapchain.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/FrameSpecificObject.h>

#include "Types.h"
#include "core/DescriptorProvider.h"

namespace trc
{
    struct GBufferCreateInfo
    {
        uvec2 size;
        ui32 maxTransparentFragsPerPixel{ 3 };
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

        struct TransparencyResources
        {
            // A storage image with format rui32
            vk::ImageView headPointerImageView;

            /**
             * A storage buffer that contains two sections:
             *
             * | section        | type/contents   |
             * |----------------------------------|
             * | allocator      | uint            |
             * | fragment list  | uvec4[]         |
             *
             * The allocator section is FRAGMENT_LIST_OFFSET bytes long
             * (padding included) and contains two uint variables:
             *    - uint nextFragmentListIndex: The atomic counter that
             *      allocates indices in the fragment list
             *    - uint maxFragmentListIndex: The number of fragment slots
             *      in the fragment list. Don't store fragments in indices
             *      higher than this number!
             *
             * After FRAGMENT_LIST_OFFSET bytes, the fragment list begins.
             * It is FRAGMENT_LIST_SIZE bytes long.
             */
            vk::Buffer allocatorAndFragmentListBuf;

            const ui32 FRAGMENT_LIST_OFFSET;
            const ui32 FRAGMENT_LIST_SIZE;
        };

        auto getTransparencyResources() const -> TransparencyResources;

        /**
         * @brief Initialize the g-buffer for rendering
         *
         * Resets the fragment allocator.
         */
        void initFrame(vk::CommandBuffer cmdBuf) const;

    private:
        //////////////////
        // Image resources

        const uvec2 size;
        const vk::Extent2D extent;
        std::vector<vkb::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

        const ui32 ATOMIC_BUFFER_SECTION_SIZE;
        const ui32 FRAG_LIST_BUFFER_SIZE;

        vkb::Image fragmentListHeadPointerImage;
        vk::UniqueImageView fragmentListHeadPointerImageView;
        vkb::Buffer fragmentListBuffer;
    };

    /**
     * @brief Resources and descriptor set for deferred renderpasses
     *
     * Provides:
     *  - binding 0: storage image rgba16f  (normals)
     *  - binding 1: storage image r32ui    (albedo)
     *  - binding 2: storage image r32ui    (materials)
     *  - binding 3: combined image sampler (depth)
     *
     *  - binding 4: storage image  (head pointer image)
     *  - binding 5: storage buffer (allocator)
     *  - binding 6: storage buffer (fragment list)
     *
     *  - binding 7: storage image rgba8 (swapchain image)
     */
    class GBufferDescriptor
    {
    public:
        /**
         * @brief Create the descriptor
         */
        GBufferDescriptor(const vkb::Device& device,
                          const vkb::Swapchain& swapchain);

        /**
         * @brief Create the descriptor and update the sets with resources
         */
        GBufferDescriptor(const vkb::Device& device,
                          const vkb::Swapchain& swapchain,
                          const vkb::FrameSpecific<GBuffer>& gBuffer);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        void update(const vkb::Swapchain& swapchain,
                    const vkb::FrameSpecific<GBuffer>& gBuffer);

    private:
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> descSets;

        FrameSpecificDescriptorProvider provider;
    };
} // namespace trc
