#include "AssetRegistry.h"



void trc::AssetRegistry::init()
{
    updateMaterialBuffer();
    createDescriptors();
}

auto trc::AssetRegistry::addGeometry(ui32 key, Geometry geo) -> Geometry&
{
    auto& result = addToMap(geometries, key, std::move(geo));
    result.id = key;

    return result;
}

auto trc::AssetRegistry::addMaterial(ui32 key, Material mat) -> Material&
{
    return addToMap(materials, key, std::move(mat));
}

auto trc::AssetRegistry::getGeometry(ui32 key) -> Geometry&
{
    return getFromMap(geometries, key);
}

auto trc::AssetRegistry::getMaterial(ui32 key) -> Material&
{
    return getFromMap(materials, key);
}

auto trc::AssetRegistry::getDescriptorSetLayout() noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto trc::AssetRegistry::getDescriptorSet() noexcept -> vk::DescriptorSet
{
    return *descSet;
}

void trc::AssetRegistry::updateMaterialBuffer()
{
    std::vector<Material> data;
    for (ui32 i = 0; i < materials.size(); i++)
    {
        const auto& matPtr = materials[i];
        if (matPtr != nullptr) {
            data.push_back(*matPtr);
        }
    }

    // The buffer should always be written to the descriptor set, which is
    // only possible if the buffer size is not zero.
    if (materials.size() == 0) {
        data.emplace_back();
    }

    materialBuffer = vkb::DeviceLocalBuffer(data, vk::BufferUsageFlagBits::eStorageBuffer);
}

void trc::AssetRegistry::createDescriptors()
{
    static const auto& device = vkb::VulkanBase::getDevice();

    descSet = {};
    descLayout = {};
    descPool = {};

    // Create pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eStorageBuffer, 1 },
    };
    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlags(), 1, poolSizes
    });

    // Create descriptor layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            MAT_BUFFER_BINDING,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment
        ),
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), layoutBindings)
    );

    // Create descriptor set
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);

    updateDescriptors();
}

void trc::AssetRegistry::updateDescriptors()
{
    static const auto& device = vkb::VulkanBase::getDevice();

    std::vector<vk::WriteDescriptorSet> writes;

    vk::DescriptorBufferInfo matBufferWrite(*materialBuffer, 0, VK_WHOLE_SIZE);
    writes.push_back(vk::WriteDescriptorSet(
        *descSet,
        MAT_BUFFER_BINDING, 0, 1,
        vk::DescriptorType::eStorageBuffer,
        {},
        &matBufferWrite
    ));

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }
}
