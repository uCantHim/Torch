#include "AssetRegistry.h"



void trc::AssetRegistry::init()
{
    createDescriptors();
}

void trc::AssetRegistry::reset()
{
    geometries = {};
    materials = {};
    imageViews = {};
    images = {};

    materialBuffer = {};

    descSet.reset();
    descLayout.reset();
    descPool.reset();

    nextGeometryIndex = 0;
    nextMaterialIndex = 0;
    nextImageIndex = 0;
}

void trc::AssetRegistry::vulkanStaticInit()
{
    constexpr size_t defaultMaterialBufferSize = sizeof(Material) * 100;

    // Create material buffer
    materialBuffer = vkb::Buffer(
        defaultMaterialBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    init();
}

void trc::AssetRegistry::vulkanStaticDestroy()
{
    reset();
}

auto trc::AssetRegistry::addGeometry(Geometry geo) -> std::pair<Ref<Geometry>, ui32>
{
    ui32 key = nextGeometryIndex++;
    auto& result = addToMap(geometries, key, std::move(geo));
    result.id = key;

    return { result, key };
}

auto trc::AssetRegistry::addMaterial(Material mat) -> std::pair<Ref<Material>, ui32>
{
    ui32 key = nextMaterialIndex++;

    return { addToMap(materials, key, mat), key };
}

auto trc::AssetRegistry::addImage(vkb::Image tex) -> std::pair<Ref<vkb::Image>, ui32>
{
    ui32 key = nextImageIndex++;
    auto& image = addToMap(images, key, std::move(tex));
    imageViews[key] = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);

    createDescriptors();

    return { image, key };
}

auto trc::AssetRegistry::getGeometry(ui32 key) -> Geometry&
{
    return getFromMap(geometries, key);
}

auto trc::AssetRegistry::getMaterial(ui32 key) -> Material&
{
    return getFromMap(materials, key);
}

auto trc::AssetRegistry::getImage(ui32 key) -> vkb::Image&
{
    return getFromMap(images, key);
}

auto trc::AssetRegistry::getDescriptorSetProvider() noexcept -> DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::AssetRegistry::updateMaterialBuffer()
{
    auto buf = reinterpret_cast<Material*>(materialBuffer.map());
    for (size_t i = 0; i < materials.size(); i++)
    {
        if (materials[i] != nullptr) {
            buf[i] = *materials[i];
        }
    }
    materialBuffer.unmap();
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
        { vk::DescriptorType::eCombinedImageSampler, max(1u, static_cast<ui32>(images.size())) },
    };
    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    });

    // Create descriptor layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            MAT_BUFFER_BINDING,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment
        ),
        vk::DescriptorSetLayoutBinding(
            IMG_DESCRIPTOR_BINDING,
            vk::DescriptorType::eCombinedImageSampler,
            images.size(),
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        ),
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), layoutBindings)
    );

    // Create descriptor set
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);

    descriptorProvider.setDescriptorSet(*descSet);
    descriptorProvider.setDescriptorSetLayout(*descLayout);

    updateDescriptors();
}

void trc::AssetRegistry::updateDescriptors()
{
    static const auto& device = vkb::VulkanBase::getDevice();

    vk::DescriptorBufferInfo matBufferWrite(*materialBuffer, 0, VK_WHOLE_SIZE);
    // Image writes
    std::vector<vk::DescriptorImageInfo> imageWrites;
    for (ui32 i = 0; i < images.size(); i++)
    {
        if (images[i] == nullptr) {
            break;
        }

        imageWrites.emplace_back(vk::DescriptorImageInfo(
            images[i]->getDefaultSampler(),
            *imageViews[i],
            vk::ImageLayout::eGeneral
        ));
    }

    std::vector<vk::WriteDescriptorSet> writes;
    writes.push_back(vk::WriteDescriptorSet(
        *descSet,
        MAT_BUFFER_BINDING, 0, 1,
        vk::DescriptorType::eStorageBuffer,
        {},
        &matBufferWrite
    ));
    if (images.size() > 0)
    {
        device->updateDescriptorSets(vk::WriteDescriptorSet(
            *descSet,
            IMG_DESCRIPTOR_BINDING, 0, imageWrites.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageWrites.data()
        ), {});
    }

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }
}
