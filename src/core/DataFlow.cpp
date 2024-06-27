#include "trc/core/DataFlow.h"



namespace trc
{

template<typename Resource, typename Access>
auto DependencyRegion::concat(
    const std::unordered_map<Resource, Access>& srcMap,
    const std::unordered_map<Resource, Access>& consumerMap,
    std::unordered_map<Resource, Access>& producerMap)
{
    using BarrierT = typename decltype(
        makeBarrier(std::declval<Access>(), std::declval<Access>())
        )::value_type;

    std::vector<BarrierT> barriers;
    for (const auto& [res, access] : srcMap)
    {
        auto it = consumerMap.find(res);
        if (it != consumerMap.end())
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
            producerMap.try_emplace(res, access);
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
