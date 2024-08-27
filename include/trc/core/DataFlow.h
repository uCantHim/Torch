#pragma once

#include <concepts>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <trc_util/Assert.h>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

template<>
struct std::hash<vk::Buffer>
{
    constexpr auto operator()(const vk::Buffer& buffer) const -> size_t {
        return std::hash<VkBuffer>{}(buffer);
    }
};

template<>
struct std::hash<vk::Image>
{
    constexpr auto operator()(const vk::Image& image) const -> size_t {
        return std::hash<VkImage>{}(image);
    }
};

namespace trc
{
    struct ImageAccess
    {
        vk::Image image;
        vk::ImageSubresourceRange subresource;

        vk::PipelineStageFlags2 pipelineStages;
        vk::AccessFlags2 access;
        vk::ImageLayout layout;

        ui32 queueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED };
    };

    struct BufferAccess
    {
        vk::Buffer buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        vk::PipelineStageFlags2 pipelineStages;
        vk::AccessFlags2 access;

        ui32 queueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED };
    };

    /**
     * A region of execution that may define resource dependencies with respect
     * to other regions.
     */
    class DependencyRegion
    {
    public:
        DependencyRegion() = default;

        /**
         * Note: If an image is consumed but is never produced by a preceding
         * dependency region, an initial layout of `vk::ImageLayout::eUndefined`
         * is assumed; meaning the image will be cleared.
         */
        void consume(const ImageAccess& image);
        void produce(const ImageAccess& image);

        void consume(const BufferAccess& buffer);
        void produce(const BufferAccess& buffer);

        /**
         * Modifies the `to` region by inserting any non-consumed resources that
         * are produced by the `from` region.
         */
        static auto genBarriers(const DependencyRegion& from,
                                DependencyRegion& to)
            -> std::pair<
                std::vector<vk::BufferMemoryBarrier2>,
                std::vector<vk::ImageMemoryBarrier2>
            >;

    private:
        /**
         * Currently adds all barriers of previous dependency regions of a
         * resource to the produced accesses of that resource in the newest
         * region.
         *
         * @param srcMap Produced resources of the previous dependency region.
         * @param dstConsumerMap Consumed resources of the next dependency region.
         * @param dstProducerMap Produced resources of the next dependency region.
         */
        template<typename Resource, typename Access>
        static auto concat(const std::unordered_map<Resource, Access>& srcMap,
                           const std::unordered_map<Resource, Access>& dstConsumerMap,
                           std::unordered_map<Resource, Access>& dstProducerMap);

        std::unordered_map<vk::Image, ImageAccess> consumedImages;
        std::unordered_map<vk::Image, ImageAccess> producedImages;

        std::unordered_map<vk::Buffer, BufferAccess> consumedBuffers;
        std::unordered_map<vk::Buffer, BufferAccess> producedBuffers;
    };

    template<typename T>
    struct RangeIntersection
    {
        bool operator==(const RangeIntersection&) const = default;

        std::optional<std::pair<T, T>> before;
        std::optional<std::pair<T, T>> overlap;
        std::optional<std::pair<T, T>> after;
    };

    /**
     * Intersects to ranges [aStart, aEnd) and [bStart, bEnd).
     *
     * Is constexpr computable!
     *
     * @param T aStart Start of range A.
     * @param T aEnd   Non-inclusive end of range A.
     * @param T bStart Start of range B.
     * @param T bEnd   Non-inclusive end of range B.
     *
     * @return RangeIntersection<T> A detailed intersection result.
     */
    template<std::totally_ordered T>
    constexpr auto intersectRanges(T aStart, T aEnd, T bStart, T bEnd)
        -> RangeIntersection<T>
    {
        auto left = glm::min(aStart, bStart);
        auto right = glm::max(aEnd, bEnd);
        auto begin = glm::max(aStart, bStart);
        auto end = glm::min(aEnd, bEnd);

        RangeIntersection<T> res;

        // Include `begin == end` because it shouldn't have an overlap
        if (begin >= end) {
            std::swap(begin, end);
        }
        else {
            res.overlap = { begin, end };
        }
        if (left != begin) res.before = { left, begin };
        if (end != right)  res.after = { end, right };

        return res;
    };

    /**
     * @brief Compute the overlap of two image subresource ranges
     *
     * @return std::optional<vk::ImageSubresourceRange> Nothing if the ranges
     *         do not overlap at all.
     */
    constexpr auto intersectSubresourceRanges(const vk::ImageSubresourceRange& a,
                                              const vk::ImageSubresourceRange& b)
        -> std::optional<vk::ImageSubresourceRange>
    {
        auto isColor = [](auto a) { return !!(a & vk::ImageAspectFlagBits::eColor); };

        // Compute aspect mask overlap
        if (isColor(a.aspectMask) != isColor(b.aspectMask)) {
            return std::nullopt;
        }
        auto aspect = a.aspectMask | b.aspectMask;

        // Compute array layer overlap
        auto [pre, layerOverlap, post] = intersectRanges(
            a.baseArrayLayer, a.baseArrayLayer + a.layerCount,
            b.baseArrayLayer, b.baseArrayLayer + b.layerCount
        );
        if (!layerOverlap) {
            return std::nullopt;
        }

        // Compute mip-level overlap
        auto [_pre, levelOverlap, _post] = intersectRanges(
            a.baseMipLevel, a.baseMipLevel + a.levelCount,
            b.baseMipLevel, b.baseMipLevel + b.levelCount
        );
        if (!levelOverlap) {
            return std::nullopt;
        }

        // Return the overlap
        auto [baseLevel, endLevel] = *levelOverlap;
        auto [baseLayer, endLayer] = *layerOverlap;
        return vk::ImageSubresourceRange{
            aspect,
            baseLevel, endLevel - baseLevel,
            baseLayer, endLayer - baseLayer
        };
    };

    /**
     * @brief Build an image memory barrier between two accesses
     */
    constexpr auto makeBarrier(const ImageAccess& from, const ImageAccess& to)
        -> std::optional<vk::ImageMemoryBarrier2>
    {
        if (from.image != to.image) {
            return std::nullopt;
        }
        auto subres = intersectSubresourceRanges(from.subresource, to.subresource);
        if (!subres) {
            return std::nullopt;
        }

        return vk::ImageMemoryBarrier2{
            from.pipelineStages,
            from.access,
            to.pipelineStages,
            to.access,
            from.layout,
            to.layout,
            from.queueFamilyIndex,
            to.queueFamilyIndex,
            from.image, *subres
        };
    }

    constexpr auto makeBarrier(const BufferAccess& from, const BufferAccess& to)
        -> std::optional<vk::BufferMemoryBarrier2>
    {
        if (from.buffer != to.buffer) {
            return std::nullopt;
        }

        // Calculate buffer range
        auto [left, overlap, right] = intersectRanges(from.offset, from.offset + from.size,
                                                      to.offset, to.offset + to.size);
        if (!overlap) {
            return std::nullopt;
        }

        // Generate barrier for the overlapping range
        const auto offset = overlap->first;
        const auto size = overlap->second - overlap->first;
        return vk::BufferMemoryBarrier2{
            from.pipelineStages, from.access,
            to.pipelineStages, to.access,
            from.queueFamilyIndex, to.queueFamilyIndex,
            from.buffer, offset, size
        };
    }

    constexpr auto makeUnion(const ImageAccess& a, const ImageAccess& b)
        -> ImageAccess
    {
        assert_arg(a.image == b.image);
        assert_arg(a.layout == b.layout);
        assert_arg(a.queueFamilyIndex == a.queueFamilyIndex);

        auto res = a;
        res.subresource.aspectMask |= b.subresource.aspectMask;

        // Array layers
        res.subresource.baseArrayLayer = glm::min(a.subresource.baseArrayLayer,
                                                  b.subresource.baseArrayLayer);
        res.subresource.layerCount =
            glm::max(
                a.subresource.baseArrayLayer + a.subresource.layerCount,
                b.subresource.baseArrayLayer + b.subresource.layerCount
            ) - res.subresource.baseArrayLayer;

        // Mip levels
        res.subresource.baseMipLevel = glm::min(a.subresource.baseMipLevel,
                                                b.subresource.baseMipLevel);
        res.subresource.levelCount =
            glm::max(
                a.subresource.baseMipLevel + a.subresource.levelCount,
                b.subresource.baseMipLevel + b.subresource.levelCount
            ) - res.subresource.baseMipLevel;

        return res;
    }

    constexpr auto makeUnion(const BufferAccess& a, const BufferAccess& b)
        -> BufferAccess
    {
        assert_arg(a.buffer == b.buffer);
        assert_arg(a.queueFamilyIndex == a.queueFamilyIndex);

        const auto offset = glm::min(a.offset, b.offset);
        const auto end = glm::max(a.offset + a.size, b.offset + b.size);
        return BufferAccess{
            a.buffer,
            offset,
            end - offset,
            a.pipelineStages | b.pipelineStages,
            a.access | b.access,
            a.queueFamilyIndex,
        };
    }
} // namespace

// Unit tests for `intersectRanges`.
//
// Notation:
//   aStart : [
//   aEnd   : ]
//   bStart : (
//   bEnd   : )
//
// [ ( ] )
static_assert(trc::intersectRanges(0, 5, 3, 8).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(0, 5, 3, 8).overlap == std::pair{ 3, 5 });
static_assert(trc::intersectRanges(0, 5, 3, 8).after   == std::pair{ 5, 8 });

// ( [ ) ]
static_assert(trc::intersectRanges(3, 8, 0, 5).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(3, 8, 0, 5).overlap == std::pair{ 3, 5 });
static_assert(trc::intersectRanges(3, 8, 0, 5).after   == std::pair{ 5, 8 });

// [ ( ) ]
static_assert(trc::intersectRanges(0, 8, 3, 5).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(0, 8, 3, 5).overlap == std::pair{ 3, 5 });
static_assert(trc::intersectRanges(0, 8, 3, 5).after   == std::pair{ 5, 8 });

// ( [ ] )
static_assert(trc::intersectRanges(3, 5, 0, 8).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(3, 5, 0, 8).overlap == std::pair{ 3, 5 });
static_assert(trc::intersectRanges(3, 5, 0, 8).after   == std::pair{ 5, 8 });

// [ ] ( )
static_assert(trc::intersectRanges(0, 3, 5, 8).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(0, 3, 5, 8).overlap == std::nullopt);
static_assert(trc::intersectRanges(0, 3, 5, 8).after   == std::pair{ 5, 8 });

// ( ) [ ]
static_assert(trc::intersectRanges(5, 8, 0, 3).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(5, 8, 0, 3).overlap == std::nullopt);
static_assert(trc::intersectRanges(5, 8, 0, 3).after   == std::pair{ 5, 8 });

// [ ]
//   ( )
static_assert(trc::intersectRanges(0, 5, 5, 8).before  == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(0, 5, 5, 8).overlap == std::nullopt);
static_assert(trc::intersectRanges(0, 5, 5, 8).after   == std::pair{ 5, 8 });

// ( )
//   [ ]
static_assert(trc::intersectRanges(5, 8, 0, 5).before  == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(5, 8, 0, 5).overlap == std::nullopt);
static_assert(trc::intersectRanges(5, 8, 0, 5).after   == std::pair{ 5, 8 });

// [ ]
// (   )
static_assert(trc::intersectRanges(0, 5, 0, 8).before  == std::nullopt);
static_assert(trc::intersectRanges(0, 5, 0, 8).overlap == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(0, 5, 0, 8).after   == std::pair{ 5, 8 });

// [   ]
// ( )
static_assert(trc::intersectRanges(0, 8, 0, 5).before  == std::nullopt);
static_assert(trc::intersectRanges(0, 8, 0, 5).overlap == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(0, 8, 0, 5).after   == std::pair{ 5, 8 });

// [   ]
//   ( )
static_assert(trc::intersectRanges(0, 8, 5, 8).before  == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(0, 8, 5, 8).overlap == std::pair{ 5, 8 });
static_assert(trc::intersectRanges(0, 8, 5, 8).after   == std::nullopt);

//   [ ]
// (   )
static_assert(trc::intersectRanges(5, 8, 0, 8).before  == std::pair{ 0, 5 });
static_assert(trc::intersectRanges(5, 8, 0, 8).overlap == std::pair{ 5, 8 });
static_assert(trc::intersectRanges(5, 8, 0, 8).after   == std::nullopt);

// [   ]
// (   )
static_assert(trc::intersectRanges(0, 8, 0, 8).before  == std::nullopt);
static_assert(trc::intersectRanges(0, 8, 0, 8).overlap == std::pair{ 0, 8 });
static_assert(trc::intersectRanges(0, 8, 0, 8).after   == std::nullopt);

// First range empty
static_assert(trc::intersectRanges(0, 0, 0, 3).before  == std::nullopt);
static_assert(trc::intersectRanges(0, 0, 0, 3).overlap == std::nullopt);
static_assert(trc::intersectRanges(0, 0, 0, 3).after   == std::pair{ 0, 3 });

// Second range empty
static_assert(trc::intersectRanges(0, 3, 3, 3).before  == std::pair{ 0, 3 });
static_assert(trc::intersectRanges(0, 3, 3, 3).overlap == std::nullopt);
static_assert(trc::intersectRanges(0, 3, 3, 3).after   == std::nullopt);

// Both ranges empty
static_assert(trc::intersectRanges(0, 0, 0, 0).before  == std::nullopt);
static_assert(trc::intersectRanges(0, 0, 0, 0).overlap == std::nullopt);
static_assert(trc::intersectRanges(0, 0, 0, 0).after   == std::nullopt);
