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

    imageLayout = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .build(device);
    tlasLayout = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .build(device);
}

auto RaygenDescriptorPool::getTlasDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *tlasLayout;
}

auto RaygenDescriptorPool::getImageDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *imageLayout;
}

auto RaygenDescriptorPool::allocateTlasDescriptorSet(const TLAS& tlas)
    -> vk::UniqueDescriptorSet
{
    auto set = std::move(device->allocateDescriptorSetsUnique({ *pool, *tlasLayout })[0]);

    vk::StructureChain tlasWrite{
        vk::WriteDescriptorSet(*set, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR),
        vk::WriteDescriptorSetAccelerationStructureKHR(*tlas)
    };
    device->updateDescriptorSets({ tlasWrite.get() }, {});

    return set;
}

auto RaygenDescriptorPool::allocateImageDescriptorSet(vk::ImageView imageView)
    -> vk::UniqueDescriptorSet
{
    auto set = std::move(device->allocateDescriptorSetsUnique({ *pool, *tlasLayout })[0]);

    vk::DescriptorImageInfo image({}, imageView, vk::ImageLayout::eGeneral);
    vk::WriteDescriptorSet imageWrite(*set, 1, 0, vk::DescriptorType::eStorageImage, image);
    device->updateDescriptorSets({ imageWrite }, {});

    return set;
}

} // namespace trc::rt
