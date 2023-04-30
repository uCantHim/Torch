#include "trc/text/GlyphMap.h"

#include "trc/base/Barriers.h"
#include "trc/base/Buffer.h"



trc::GlyphMap::GlyphMap(const Device& device, const DeviceMemoryAllocator& alloc)
    :
    device(device),
    image(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8Unorm,
            { MAP_WIDTH, MAP_HEIGHT, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        ),
        alloc
    )
{
    // Set initial image layout
    device.executeCommands(QueueType::graphics, [&](auto cmdBuf){
        barrier(cmdBuf, vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eHost, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        });
    });
}

auto trc::GlyphMap::addGlyph(const GlyphMeta& glyph) -> UvRectangle
{
    const auto& [data, size] = glyph.pixelData;
    if (data.empty()) {
        return { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    }

    if (offset.x + size.x > MAP_WIDTH)
    {
        offset.x = 0;
        offset.y += maxHeight;
        offset.y += 1; // A small spacing between glyphs to account for uv-inaccuracy
        maxHeight = 0;
    }
    if (offset.y + size.y > MAP_HEIGHT) {
        throw std::out_of_range("No more space in glyph map!");
    }

    writeDataToImage(
        data,
        ImageSize{ .offset={ offset.x, offset.y, 0 }, .extent={ size.x, size.y, 1 } }
    );

    GLM_CONSTEXPR vec2 mapSize{ MAP_WIDTH, MAP_HEIGHT };
    vec2 ll{ vec2(offset) / mapSize };
    vec2 ur{ (vec2(offset) + vec2(size)) / mapSize };

    maxHeight = glm::max(maxHeight, size.y);
    offset.x += size.x;
    offset.x += 1; // A small spacing between glyphs to account for uv-inaccuracy

    return { ll, ur };
}

auto trc::GlyphMap::getGlyphImage() -> Image&
{
    return image;
}

void trc::GlyphMap::writeDataToImage(const std::vector<ui8>& data, const ImageSize& dstArea)
{
    Buffer buffer(
        device, data,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    device.executeCommands(QueueType::transfer, [&](vk::CommandBuffer cmdBuf) {
        constexpr auto externalStages = vk::PipelineStageFlagBits2::eTransfer;
        constexpr auto externalAccess = vk::AccessFlagBits2::eTransferWrite;

        barrier(cmdBuf, vk::ImageMemoryBarrier2{
            externalStages, externalAccess,
            vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        });

        image.writeData(cmdBuf, *buffer, dstArea);

        barrier(cmdBuf, vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            externalStages, externalAccess,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        });
    });
}
