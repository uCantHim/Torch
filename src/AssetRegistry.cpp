#include "AssetRegistry.h"

#include "PipelineRegistry.h"



void trc::AssetRegistry::init()
{
    constexpr size_t defaultMaterialBufferSize = sizeof(Material) * 100;

    // Create material buffer
    materialBuffer = vkb::Buffer(
        defaultMaterialBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    // Add default assets
    addMaterial({});
    updateMaterials();
    addImage(vkb::makeImage2D(vkb::getDevice(), vec4(1.0f)));

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

auto trc::AssetRegistry::addGeometry(Geometry geo) -> GeometryID
{
    GeometryID key = nextGeometryIndex++;
    addToMap(geometries, key, std::move(geo));

    return key;
}

auto trc::AssetRegistry::addMaterial(Material mat) -> MaterialID
{
    MaterialID key = nextMaterialIndex++;
    addToMap(materials, key, mat);

    return key;
}

auto trc::AssetRegistry::addImage(vkb::Image tex) -> TextureID
{
    TextureID key = nextImageIndex++;
    auto& image = addToMap(images, key, std::move(tex));
    imageViews[key] = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);

    createDescriptors();
    PipelineRegistry::recreateAll();

    return key;
}

auto trc::AssetRegistry::getGeometry(GeometryID key) -> Maybe<Geometry*>
{
    return &getFromMap(geometries, key);
}

auto trc::AssetRegistry::getMaterial(MaterialID key) -> Maybe<Material*>
{
    return &getFromMap(materials, key);
}

auto trc::AssetRegistry::getImage(TextureID key) -> Maybe<vkb::Image*>
{
    return &getFromMap(images, key);
}

auto trc::AssetRegistry::getDescriptorSetProvider() noexcept -> DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::AssetRegistry::updateMaterials()
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
        { vk::DescriptorType::eCombinedImageSampler, glm::max(1u, static_cast<ui32>(images.size())) },
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
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, 1, &*descLayout })[0]);

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
