#pragma once

#include "trc/base/Buffer.h"
#include "trc/base/Image.h"
#include "trc/base/FrameSpecificObject.h"

#include "trc/Types.h"
#include "trc/core/DescriptorProvider.h"

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

            /**
             * The material image contains 4 8-bit unorm floats, with the
             * following semantics:
             *
             * --------------------------------------------------------------------
             * | 0-7                    | 8-15        | 16-23          | 24-31    |
             * |------------------------|-------------|----------------|----------|
             * | specular coefficient   | roughness   | metallicness   | unused   |
             * --------------------------------------------------------------------
             */
            eMaterials,
            eDepth,

            NUM_IMAGES
        };

        GBuffer(const Device& device, const GBufferCreateInfo& size);

        auto getSize() const -> uvec2;
        auto getExtent() const -> vk::Extent2D;

        auto getImage(Image imageType) -> trc::Image&;
        auto getImage(Image imageType) const -> const trc::Image&;
        auto getImageView(Image imageType) const -> vk::ImageView;

        struct TransparencyResources
        {
            // A storage image with format rui32
            vk::ImageView headPointerImageView;

            /**
             * A storage buffer that contains two sections:
             *
             * | section        | type            | contents         |
             * |----------------------------------|------------------|
             * | allocator      | uint            | next frag index  |
             * | allocator      | uint            | max frag index   |
             * | fragment list  | uvec4[]         | fragment data    |
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
        std::vector<trc::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

        const ui32 ATOMIC_BUFFER_SECTION_SIZE;
        const ui32 FRAG_LIST_BUFFER_SIZE;

        trc::Image fragmentListHeadPointerImage;
        vk::UniqueImageView fragmentListHeadPointerImageView;
        Buffer fragmentListBuffer;
    };

    enum class GBufferDescriptorBinding
    {
        // Storage image containing one surface normal per pixel
        //
        // GLSL format: `rgba16f image2D`
        eNormalImage,

        // Storage image containing one albedo (unshaded color) value per pixel
        //
        // GLSL format: `rgba8 image2D`
        eAlbedoImage,

        // Storage image containing all material parameters required for shading
        // per pixel
        //
        // GLSL format: `rgba8 image2D`
        eMaterialParamsImage,

        // 2D sampler over the depth buffer
        //
        // GLSL format: `sampler2D`
        eDepthImage,

        // Storage image containing the pixels' pointer to the start of its
        // linked list of transparent fragments.
        //
        // GLSL format: `r32ui uimage2D`
        eTpFragHeadPointerImage,

        // Very small storage buffer containing two integer values; one used as
        // an atomic counter and one as a constant upper bound for that index
        // (maximum number of entries in the list).
        //
        // Used to allocate entries in the fragment list during the collection
        // of transparent fragments.
        //
        // GLSL format: `buffer { uint nextIndex; uint maxIndex; }`
        eTpFragListEntryAllocator,

        // Storage buffer containing all transparent fragments in the frame.
        // Initially indexed by the contents of the head pointer image.
        //
        // GLSL format: `buffer { uvec4 fragmentList[]; }`
        eTpFragListBuffer,
    };

    /**
     * @brief Resources and descriptor set for deferred renderpasses
     *
     * See `GBufferDescriptorBinding` for a list of bindings provided by this
     * descriptor.
     */
    class GBufferDescriptor
    {
    public:
        /**
         * @brief Create the descriptor
         */
        GBufferDescriptor(const Device& device,
                          const FrameClock& frameClock);

        /**
         * @brief Create the descriptor and update the sets with resources
         */
        GBufferDescriptor(const Device& device,
                          const FrameSpecific<GBuffer>& gBuffer);

        void update(const Device& device,
                    const FrameSpecific<GBuffer>& gBuffer);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * @return ui32 The specified binding's index in the descriptor set.
         */
        static constexpr auto getBindingIndex(GBufferDescriptorBinding binding) -> ui32 {
            return static_cast<ui32>(binding);
        }

    private:
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        FrameSpecific<vk::UniqueDescriptorSet> descSets;

        FrameSpecificDescriptorProvider provider;
    };
} // namespace trc
