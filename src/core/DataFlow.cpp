#include "trc/core/DataFlow.h"



namespace trc
{

auto makeResourceInit(const ImageAccess& access)
    -> ImageAccess
{
    return {
        .image=access.image,
        .subresource=access.subresource,
        .pipelineStages=vk::PipelineStageFlagBits2::eTopOfPipe,
        .access=vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
        .layout=vk::ImageLayout::eUndefined,
    };
}

auto makeResourceInit(const BufferAccess& access)
    -> BufferAccess
{
    return {
        .buffer=access.buffer,
        .offset=access.offset,
        .size=access.size,
        .pipelineStages=vk::PipelineStageFlagBits2::eTopOfPipe,
        .access=vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    };
}

template<typename Resource, typename Access>
auto DependencyRegion::concat(
    const std::unordered_map<Resource, Access>& srcMap,
    const std::unordered_map<Resource, Access>& dstConsumerMap,
    std::unordered_map<Resource, Access>& dstProducerMap)
{
    using BarrierT = typename decltype(
        makeBarrier(std::declval<Access>(), std::declval<Access>())
        )::value_type;

    std::vector<BarrierT> barriers;

    // Create barriers from all resources provided by the source map to where
    // they are consumed. If they are not consumed by the destination map, they
    // are propagated to the destination map as if they were produced there.
    for (const auto& [res, access] : srcMap)
    {
        auto it = dstConsumerMap.find(res);
        if (it != dstConsumerMap.end())
        {
            // Resource is consumed. Create a barrier.
            const auto barrier = makeBarrier(access, it->second);
            if (barrier) {
                barriers.emplace_back(*barrier);
            }
        }
        else {
            // Resource is not consumed. Add it to the next region.
            //
            // TODO: Some sort of intersection should be calculated here in case
            // the next region produces a subset of what the previous region
            // produces.
            dstProducerMap.try_emplace(res, access);
        }
    }

    // Create barriers for all resources that are consumed by the destination
    // map but not provided by the source map. Use default initializers for
    // these resources.
    for (const auto& [res, access] : dstConsumerMap)
    {
        if (!srcMap.contains(res))
        {
            const auto barrier = makeBarrier(makeResourceInit(access), access);
            if (barrier) {
                barriers.emplace_back(*barrier);
            }
        }
    }

    return barriers;
}

void DependencyRegion::consume(const ImageAccess& access)
{
    auto [it, success] = consumedImages.try_emplace(access.image, access);
    if (!success) {
        it->second = makeUnion(it->second, access);
    }
}

void DependencyRegion::produce(const ImageAccess& access)
{
    auto [it, success] = producedImages.try_emplace(access.image, access);
    if (!success) {
        it->second = makeUnion(it->second, access);
    }
}

void DependencyRegion::consume(const BufferAccess& access)
{
    auto [it, success] = consumedBuffers.try_emplace(access.buffer, access);
    if (!success) {
        it->second = makeUnion(it->second, access);
    }
}

void DependencyRegion::produce(const BufferAccess& access)
{
    producedBuffers.try_emplace(access.buffer, access);
}

auto DependencyRegion::genBarriers(
    const DependencyRegion& from,
    DependencyRegion& to)
    -> std::pair<
        std::vector<vk::BufferMemoryBarrier2>,
        std::vector<vk::ImageMemoryBarrier2>
    >
{
    auto bufferBarriers = concat(from.producedBuffers, to.consumedBuffers, to.producedBuffers);
    auto imageBarriers = concat(from.producedImages, to.consumedImages, to.producedImages);

    return { bufferBarriers, imageBarriers };
}

} // namespace trc
