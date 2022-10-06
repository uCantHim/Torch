#include "trc/ray_tracing/RaygenDescriptor.h"

#include "trc/DescriptorSetUtils.h"
#include "trc/core/Instance.h"



namespace trc::rt
{

RaygenDescriptorPool::RaygenDescriptorPool(const Instance& instance, ui32 maxDescriptorSets)
    :
    device(instance.getDevice())
{
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eAccelerationStructureKHR, maxDescriptorSets },
        { vk::DescriptorType::eStorageImage, maxDescriptorSets },
    };
    pool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        maxDescriptorSets,
        poolSizes
    });

    layout = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .build(device);
}

auto RaygenDescriptorPool::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *layout;
}

auto RaygenDescriptorPool::allocateDescriptorSet(const TLAS& tlas, vk::ImageView outputImageView)
    -> vk::UniqueDescriptorSet
{
    auto set = std::move(device->allocateDescriptorSetsUnique({ *pool, *layout })[0]);

    vk::StructureChain tlasWrite{
        vk::WriteDescriptorSet(*set, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR),
        vk::WriteDescriptorSetAccelerationStructureKHR(*tlas)
    };

    vk::DescriptorImageInfo image({}, outputImageView, vk::ImageLayout::eGeneral);
    vk::WriteDescriptorSet imageWrite(*set, 1, 0, vk::DescriptorType::eStorageImage, image);

    device->updateDescriptorSets({ tlasWrite.get(), imageWrite }, {});

    return set;
}

auto RaygenDescriptorPool::allocateFrameSpecificDescriptorSet(
    const TLAS& tlas,
    vkb::FrameSpecific<vk::ImageView> outputImageView)
    -> vkb::FrameSpecific<vk::UniqueDescriptorSet>
{
    return {
        outputImageView.getFrameClock(),
        [&](ui32 i) {
            return allocateDescriptorSet(tlas, outputImageView.getAt(i));
        }
    };
}

} // namespace trc::rt
