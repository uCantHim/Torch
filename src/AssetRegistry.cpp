#include "AssetRegistry.h"



void trc::AssetRegistry::init()
{
    images.reserve(10);
    imageViews.reserve(10);
    addImage(vkb::Image("/home/nicola/dotfiles/arch_3D_simplistic.png"));

    updateMaterialBuffer();
    createDescriptors();
}

auto trc::AssetRegistry::addGeometry(Geometry geo) -> std::pair<Geometry*, ui32>
{
    ui32 key = nextGeometryIndex++;
    auto& result = addToMap(geometries, key, std::move(geo));
    result.id = key;

    return { &result, key };
}

auto trc::AssetRegistry::addMaterial(Material mat) -> std::pair<Material*, ui32>
{
    ui32 key = nextMaterialIndex++;

    return { &addToMap(materials, key, std::move(mat)), key };
}

auto trc::AssetRegistry::addImage(vkb::Image tex) -> std::pair<vkb::Image*, ui32>
{
    ui32 key = nextImageIndex++;
    auto& image = addToMap(images, key, std::move(tex));
    imageViews[key] = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Snorm);

    //createDescriptors();

    return { &image, key };
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
        { vk::DescriptorType::eCombinedImageSampler, max(1u, static_cast<ui32>(images.size())) },
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

        imageWrites.push_back(vk::DescriptorImageInfo(
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
