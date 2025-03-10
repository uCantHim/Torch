#include "trc/GBuffer.h"

#include <trc_util/Padding.h>

#include "trc/DescriptorSetUtils.h"
#include "trc/Camera.h"
#include "trc/base/Barriers.h"



trc::GBuffer::GBuffer(const Device& device, const GBufferCreateInfo& info)
    :
    size(info.size),
    extent(size.x, size.y),
    ATOMIC_BUFFER_SECTION_SIZE(util::pad(
        sizeof(ui32) * 3,  // current index, max index, clear value
        device.getPhysicalDevice().properties.limits.minStorageBufferOffsetAlignment
    )),
    FRAG_LIST_BUFFER_SIZE(
        sizeof(uvec4) * info.maxTransparentFragsPerPixel * info.size.x * info.size.y
    ),
    fragmentListHeadPointerImage(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D, vk::Format::eR32Uint,
            vk::Extent3D(size.x, size.y, 1),
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst
        ),
        DefaultDeviceMemoryAllocator()
    ),
    fragmentListHeadPointerImageView(
        fragmentListHeadPointerImage.createView(vk::ImageViewType::e2D, vk::Format::eR32Uint)
    ),
    fragmentListBuffer(
        device,
        ATOMIC_BUFFER_SECTION_SIZE + FRAG_LIST_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eTransferDst
            | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    )
{
    device.setDebugName(*fragmentListHeadPointerImage, "G-Buffer fragment list head pointer image");
    device.setDebugName(*fragmentListHeadPointerImageView, "G-Buffer fragment list head pointer image view");
    device.setDebugName(*fragmentListBuffer, "G-Buffer fragment list buffer");

    // Clear fragment head pointer image
    device.executeCommands(QueueType::graphics, [this](auto cmdBuf)
    {
        barrier(cmdBuf, vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eHost, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eClear, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *fragmentListHeadPointerImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        });
        cmdBuf.clearColorImage(
            *fragmentListHeadPointerImage, vk::ImageLayout::eGeneral,
            vk::ClearColorValue(std::array<ui32, 4>{ ~0u }),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    });

    // Clear atomic counter buffer
    device.executeCommands(QueueType::transfer, [&, this](vk::CommandBuffer cmdBuf)
    {
        const ui32 MAX_FRAGS = info.maxTransparentFragsPerPixel * info.size.x * info.size.y;
        cmdBuf.updateBuffer<ui32>(*fragmentListBuffer, 0, { 0, MAX_FRAGS, 0, });
    });


    ///////////////////////////
    // Create attachment images

    images.reserve(Image::NUM_IMAGES);
    const auto colorUsage = vk::ImageUsageFlagBits::eColorAttachment
                            | vk::ImageUsageFlagBits::eInputAttachment
                            | vk::ImageUsageFlagBits::eStorage;
    const auto depthUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment
                            | vk::ImageUsageFlagBits::eTransferSrc
                            | vk::ImageUsageFlagBits::eInputAttachment
                            | vk::ImageUsageFlagBits::eSampled;

    // Normals
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR16G16B16A16Sfloat,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        DefaultDeviceMemoryAllocator()
    );

    // Albedo
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        DefaultDeviceMemoryAllocator()
    );

    // Material
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        DefaultDeviceMemoryAllocator()
    );

    // Depth
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eD24UnormS8Uint,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            depthUsage
        ),
        DefaultDeviceMemoryAllocator()
    );

    imageViews.push_back(getImage(Image::eNormals).createView());
    imageViews.push_back(getImage(Image::eAlbedo).createView());
    imageViews.push_back(getImage(Image::eMaterials).createView());
    imageViews.push_back(getImage(Image::eDepth).createView(vk::ImageAspectFlagBits::eDepth));

    device.setDebugName(*images.at(Image::eNormals), "G-Buffer normal image");
    device.setDebugName(*images.at(Image::eAlbedo), "G-Buffer albedo image");
    device.setDebugName(*images.at(Image::eMaterials), "G-Buffer material index image");
    device.setDebugName(*images.at(Image::eDepth), "G-Buffer depth image");
}

auto trc::GBuffer::getSize() const -> uvec2
{
    return size;
}

auto trc::GBuffer::getExtent() const -> vk::Extent2D
{
    return extent;
}

auto trc::GBuffer::getImage(Image imageType) -> trc::Image&
{
    return images[imageType];
}

auto trc::GBuffer::getImage(Image imageType) const -> const trc::Image&
{
    return images[imageType];
}

auto trc::GBuffer::getImageView(Image imageType) const -> vk::ImageView
{
    return *imageViews[imageType];
}

auto trc::GBuffer::getTransparencyResources() const -> TransparencyResources
{
    return {
        *fragmentListHeadPointerImageView,
        *fragmentListBuffer,
        ATOMIC_BUFFER_SECTION_SIZE,
        FRAG_LIST_BUFFER_SIZE
    };
}

void trc::GBuffer::initFrame(vk::CommandBuffer cmdBuf) const
{
    // Cheat: Copy a constantly zero-valued byte to the first byte in the buffer
    cmdBuf.copyBuffer(*fragmentListBuffer, *fragmentListBuffer,
                      vk::BufferCopy(sizeof(ui32) * 2, 0, sizeof(ui32)));
    fragmentListBuffer.barrier(
        cmdBuf,
        0, sizeof(ui32),
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader,
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite
    );
}



// -------------------------------- //
//       G-Buffer descriptor        //
// -------------------------------- //

trc::GBufferDescriptor::GBufferDescriptor(
    const Device& device,
    ui32 maxDescriptorSets)
{
    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eStorageImage, 5 },
        { vk::DescriptorType::eStorageBuffer, 2 },
        { vk::DescriptorType::eCombinedImageSampler, 1 },
    };
    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        maxDescriptorSets,
        poolSizes
    });

    auto shaderStages = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eRaygenKHR;

    // Layout
    descLayout = buildDescriptorSetLayout()
        // G-Buffer images
        .addBinding(vk::DescriptorType::eStorageImage, 1, shaderStages)
        .addBinding(vk::DescriptorType::eStorageImage, 1, shaderStages)
        .addBinding(vk::DescriptorType::eStorageImage, 1, shaderStages)
        .addBinding(vk::DescriptorType::eCombinedImageSampler, 1, shaderStages)
        // Fragment list head pointer image
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eFragment
                                                          | shaderStages)
        // Fragment list allocator
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment
                                                           | shaderStages)
        // Fragment list
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment
                                                           | shaderStages)
        // Swapchain image
        .addBinding(vk::DescriptorType::eStorageImage, 1, shaderStages)
        .build(device);

    // Sets
}

auto trc::GBufferDescriptor::makeDescriptorSet(const Device& device, const GBuffer& g)
    -> vk::UniqueDescriptorSet
{
    auto set = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);

    vk::ImageView imageViews[]{
        g.getImageView(GBuffer::eNormals),
        g.getImageView(GBuffer::eAlbedo),
        g.getImageView(GBuffer::eMaterials),
        g.getImageView(GBuffer::eDepth),
    };
    vk::Sampler depthSampler = g.getImage(GBuffer::eDepth).getDefaultSampler();
    const auto transparent = g.getTransparencyResources();

    // Write to the descriptor set
    vk::DescriptorImageInfo imageInfos[]{
        { {},           imageViews[0],                    vk::ImageLayout::eGeneral },
        { {},           imageViews[1],                    vk::ImageLayout::eGeneral },
        { {},           imageViews[2],                    vk::ImageLayout::eGeneral },
        { depthSampler, imageViews[3],                    vk::ImageLayout::eShaderReadOnlyOptimal },
        { {},           transparent.headPointerImageView, vk::ImageLayout::eGeneral },
    };
    vk::DescriptorBufferInfo bufferInfos[]{
        { transparent.allocatorAndFragmentListBuf, 0, transparent.FRAGMENT_LIST_OFFSET },
        { transparent.allocatorAndFragmentListBuf, transparent.FRAGMENT_LIST_OFFSET,
                                                   transparent.FRAGMENT_LIST_SIZE },
    };

    std::vector<vk::WriteDescriptorSet> writes{
        { *set, 0, 0, vk::DescriptorType::eStorageImage,         imageInfos[0] },
        { *set, 1, 0, vk::DescriptorType::eStorageImage,         imageInfos[1] },
        { *set, 2, 0, vk::DescriptorType::eStorageImage,         imageInfos[2] },
        { *set, 3, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos[3] },

        { *set, 4, 0, vk::DescriptorType::eStorageImage,         imageInfos[4] },
        { *set, 5, 0, vk::DescriptorType::eStorageBuffer,        {}, bufferInfos[0] },
        { *set, 6, 0, vk::DescriptorType::eStorageBuffer,        {}, bufferInfos[1] },
    };

    device->updateDescriptorSets(writes, {});

    return set;
}

auto trc::GBufferDescriptor::getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}
