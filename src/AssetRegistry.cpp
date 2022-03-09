#include "AssetRegistry.h"

#include <vkb/ImageUtils.h>

#include "core/Instance.h"
#include "GeometryRegistry.h"
#include "MaterialRegistry.h"
#include "TextureRegistry.h"
#include "ray_tracing/RayPipelineBuilder.h"



trc::AssetRegistry::AssetRegistry(
    const Instance& instance,
    const AssetRegistryCreateInfo& info)
    :
    device(instance.getDevice()),
    config(addDefaultValues(info)),
    modules(AssetRegistryModuleCreateInfo{
        .device=instance.getDevice(),
        .geometryBufferUsage=config.geometryBufferUsage,
        .enableRayTracing=config.enableRayTracing,
    }),
    fontData(instance),
    animationStorage(instance)
{
    createDescriptors();

    // Add default assets
    add(MaterialDeviceHandle{ .performLighting=false });
    add({ "trc_default_texture", { 1, 1 }, vkb::makeSinglePixelImageData(vec4(1.0f)).pixels });

    writeDescriptors();
}

auto trc::AssetRegistry::add(const GeometryData& data) -> LocalID<Geometry>
{
    const auto id = modules.get<AssetRegistryModule<Geometry>>().add(data);

    if (config.enableRayTracing)
    {
        writeDescriptors();
    }

    return id;
}

auto trc::AssetRegistry::add(const MaterialDeviceHandle& data) -> LocalID<Material>
{
    const auto id = modules.get<AssetRegistryModule<Material>>().add(data);

    updateMaterials();

    return id;
}

auto trc::AssetRegistry::add(const TextureData& tex) -> LocalID<Texture>
{
    const auto id = modules.get<AssetRegistryModule<Texture>>().add(tex);

    writeDescriptors();

    return id;
}

auto trc::AssetRegistry::get(LocalID<Geometry> id) -> GeometryDeviceHandle
{
    return modules.get<GeometryRegistry>().getHandle(id);
}

auto trc::AssetRegistry::get(LocalID<Material> id) -> MaterialDeviceHandle
{
    return modules.get<MaterialRegistry>().getHandle(id);
}

auto trc::AssetRegistry::get(LocalID<Texture> id) -> TextureDeviceHandle
{
    return modules.get<TextureRegistry>().getHandle(id);
}

auto trc::AssetRegistry::getFonts() -> FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getFonts() const -> const FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getAnimations() -> AnimationDataStorage&
{
    return animationStorage;
}

auto trc::AssetRegistry::getAnimations() const -> const AnimationDataStorage&
{
    return animationStorage;
}

auto trc::AssetRegistry::getDescriptorSetProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::AssetRegistry::updateMaterials()
{
    modules.get<MaterialRegistry>().update();
}

auto trc::AssetRegistry::addDefaultValues(const AssetRegistryCreateInfo& info)
    -> AssetRegistryCreateInfo
{
    auto result = info;

    result.materialDescriptorStages |= vk::ShaderStageFlagBits::eFragment
                                       | vk::ShaderStageFlagBits::eCompute;
    result.textureDescriptorStages |= vk::ShaderStageFlagBits::eFragment
                                      | vk::ShaderStageFlagBits::eCompute;

    if (info.enableRayTracing)
    {
        result.geometryBufferUsage |=
            vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress
            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

        result.materialDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
        result.textureDescriptorStages  |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
        result.geometryDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
    }

    return result;
}

void trc::AssetRegistry::createDescriptors()
{
    descSet = {};
    descLayout = {};
    descPool = {};

    // Create pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eCombinedImageSampler, MAX_TEXTURE_COUNT },
    };
    if (config.enableRayTracing)
    {
        // Index and vertex buffers
        poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, MAX_GEOMETRY_COUNT);
        poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, MAX_GEOMETRY_COUNT);
    }

    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1, poolSizes
    });

    // Create descriptor layout
    std::vector<vk::DescriptorBindingFlags> bindingFlags{
        {},                                              // Flags for material buffer
        vk::DescriptorBindingFlagBits::ePartiallyBound,  // Flags for textures
    };
    if (config.enableRayTracing)
    {
        // Flags for vertex- and index buffers
        bindingFlags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
        bindingFlags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
    }

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            DescBinding::eMaterials,
            vk::DescriptorType::eStorageBuffer,
            1,
            config.materialDescriptorStages
        ),
        vk::DescriptorSetLayoutBinding(
            DescBinding::eTextures,
            vk::DescriptorType::eCombinedImageSampler,
            MAX_TEXTURE_COUNT,
            config.textureDescriptorStages
        ),
    };
    if (config.enableRayTracing)
    {
        layoutBindings.push_back(vk::DescriptorSetLayoutBinding(
            DescBinding::eVertexBuffers,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            config.geometryDescriptorStages
        ));
        layoutBindings.push_back(vk::DescriptorSetLayoutBinding(
            DescBinding::eIndexBuffers,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            config.geometryDescriptorStages
        ));
    }

    vk::StructureChain layoutChain{
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Create descriptor set
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, 1, &*descLayout })[0]);

    // Update provider
    descriptorProvider.setDescriptorSetLayout(*descLayout);
    descriptorProvider.setDescriptorSet(*descSet);
}

void trc::AssetRegistry::writeDescriptors()
{
    // Material descriptor infos
    vk::DescriptorBufferInfo matBufferWrite(*materialBuffer, 0, VK_WHOLE_SIZE);

    // Collect descriptor writes
    std::vector<vk::WriteDescriptorSet> writes;

    // Bind material buffer
    writes.push_back(vk::WriteDescriptorSet(
        *descSet,
        DescBinding::eMaterials, 0, 1,
        vk::DescriptorType::eStorageBuffer,
        {},
        &matBufferWrite
    ));

    // Texture descriptor infos
    std::vector<vk::DescriptorImageInfo> imageWrites;
    for (ui32 i = 0; i < textures.size(); i++)
    {
        auto& tex = textures[LocalID<Texture>::Type(i)];
        if (tex == nullptr) break;

        imageWrites.emplace_back(vk::DescriptorImageInfo(
            tex->image.getDefaultSampler(),
            *tex->imageView,
            vk::ImageLayout::eGeneral
        ));
    }

    if (textures.size() > 0)
    {
        writes.push_back(vk::WriteDescriptorSet(
            *descSet,
            DescBinding::eTextures, 0, imageWrites.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageWrites.data()
        ));
    }

    // Geometry descriptor infos
    std::vector<vk::DescriptorBufferInfo> vertBufs;
    std::vector<vk::DescriptorBufferInfo> indexBufs;
    if (config.enableRayTracing)
    {
        for (auto& geo : geometries)
        {
            if (geo == nullptr)
            {
                vertBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
                indexBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
            }
            else
            {
                vertBufs.emplace_back(vk::DescriptorBufferInfo(*geo->vertexBuf, 0, VK_WHOLE_SIZE));
                indexBufs.emplace_back(vk::DescriptorBufferInfo(*geo->indexBuf, 0, VK_WHOLE_SIZE));
            }
        }

        if (geometries.size() > 0)
        {
            writes.push_back(vk::WriteDescriptorSet(
                *descSet, DescBinding::eVertexBuffers, 0, vertBufs.size(),
                vk::DescriptorType::eStorageBuffer,
                nullptr,
                vertBufs.data()
            ));
            writes.push_back(vk::WriteDescriptorSet(
                *descSet, DescBinding::eIndexBuffers, 0, indexBufs.size(),
                vk::DescriptorType::eStorageBuffer,
                nullptr,
                indexBufs.data()
            ));
        }
    }

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }
}
