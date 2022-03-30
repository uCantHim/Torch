#include "assets/AssetRegistry.h"

#include <vkb/ImageUtils.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "core/Instance.h"
#include "assets/GeometryRegistry.h"
#include "assets/MaterialRegistry.h"
#include "assets/TextureRegistry.h"
#include "assets/AnimationRegistry.h"
#include "ray_tracing/RayPipelineBuilder.h"



trc::AssetRegistry::AssetRegistry(
    const Instance& instance,
    const AssetRegistryCreateInfo& info)
    :
    device(instance.getDevice()),
    config(addDefaultValues(info)),
    fontData(instance)
{
    AssetRegistryModuleCreateInfo moduleCreateInfo{
        .device=instance.getDevice(),
        .geoVertexBufBinding = DescBinding::eVertexBuffers,
        .geoIndexBufBinding  = DescBinding::eIndexBuffers,
        .materialBufBinding  = DescBinding::eMaterials,
        .textureBinding      = DescBinding::eTextures,
        .animationBinding    = DescBinding::eAnimations,
        .geometryBufferUsage=config.geometryBufferUsage,
        .enableRayTracing=config.enableRayTracing && instance.hasRayTracing(),
    };

    // Add modules in the order in which they should be destroyed
    modules.addModule<MaterialRegistry>(moduleCreateInfo);
    modules.addModule<TextureRegistry>(moduleCreateInfo);
    modules.addModule<GeometryRegistry>(moduleCreateInfo);
    modules.addModule<RigRegistry>(moduleCreateInfo);
    modules.addModule<AnimationRegistry>(moduleCreateInfo);

    createDescriptors();

    // Add default assets
    add(std::make_unique<InMemorySource<Material>>(MaterialData{ .doPerformLighting=false }));
    add(std::make_unique<InMemorySource<Texture>>(
        TextureData{ { 1, 1 }, vkb::makeSinglePixelImageData(vec4(1.0f)).pixels }
    ));

    writeDescriptors();
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Geometry>> src) -> LocalID<Geometry>
{
    const auto id = modules.get<AssetRegistryModule<Geometry>>().add(std::move(src));

    if (config.enableRayTracing)
    {
        writeDescriptors();
    }

    return id;
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Material>> src) -> LocalID<Material>
{
    const auto id = modules.get<AssetRegistryModule<Material>>().add(std::move(src));

    updateMaterials();

    return id;
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Texture>> src) -> LocalID<Texture>
{
    const auto id = modules.get<AssetRegistryModule<Texture>>().add(std::move(src));

    writeDescriptors();

    return id;
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Rig>> src) -> LocalID<Rig>
{
    return modules.get<AssetRegistryModule<Rig>>().add(std::move(src));
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Animation>> src) -> LocalID<Animation>
{
    return modules.get<AssetRegistryModule<Animation>>().add(std::move(src));
}

auto trc::AssetRegistry::getFonts() -> FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getFonts() const -> const FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getDescriptorSetProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::AssetRegistry::updateMaterials()
{
    modules.get<MaterialRegistry>().update({});
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

    // Collect layout bindings
    std::vector<DescriptorLayoutBindingInfo> layoutBindings;
    modules.foreach([&layoutBindings](AssetRegistryModuleInterface& mod) {
        util::merge(layoutBindings, mod.getDescriptorLayoutBindings());
    });

    // Create pool
    descPool = makeDescriptorPool(
        device,
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,  // This flags is set automatically
        1, layoutBindings
    );

    // Create layout
    descLayout = makeDescriptorSetLayout(device, layoutBindings);

    // Create descriptor set
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, 1, &*descLayout })[0]);

    // Update provider
    descriptorProvider.setDescriptorSetLayout(*descLayout);
    descriptorProvider.setDescriptorSet(*descSet);
}

void trc::AssetRegistry::writeDescriptors()
{
    std::vector<vk::WriteDescriptorSet> writes;
    modules.foreach([&writes](AssetRegistryModuleInterface& mod) {
        util::merge(writes, mod.getDescriptorUpdates());
    });

    for (auto& write : writes) {
        write.setDstSet(*descSet);
    }

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }



    // // Collect descriptor writes
    // std::vector<vk::WriteDescriptorSet> writes;

    // // Texture descriptor infos
    // std::vector<vk::DescriptorImageInfo> imageWrites;
    // for (ui32 i = 0; i < textures.size(); i++)
    // {
    //     auto& tex = textures[LocalID<Texture>::Type(i)];
    //     if (tex == nullptr) break;

    //     imageWrites.emplace_back(vk::DescriptorImageInfo(
    //         tex->image.getDefaultSampler(),
    //         *tex->imageView,
    //         vk::ImageLayout::eGeneral
    //     ));
    // }

    // if (textures.size() > 0)
    // {
    //     writes.push_back(vk::WriteDescriptorSet(
    //         *descSet,
    //         DescBinding::eTextures, 0, imageWrites.size(),
    //         vk::DescriptorType::eCombinedImageSampler,
    //         imageWrites.data()
    //     ));
    // }

    // // Geometry descriptor infos
    // std::vector<vk::DescriptorBufferInfo> vertBufs;
    // std::vector<vk::DescriptorBufferInfo> indexBufs;
    // if (config.enableRayTracing)
    // {
    //     for (auto& geo : geometries)
    //     {
    //         if (geo == nullptr)
    //         {
    //             vertBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
    //             indexBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
    //         }
    //         else
    //         {
    //             vertBufs.emplace_back(vk::DescriptorBufferInfo(*geo->vertexBuf, 0, VK_WHOLE_SIZE));
    //             indexBufs.emplace_back(vk::DescriptorBufferInfo(*geo->indexBuf, 0, VK_WHOLE_SIZE));
    //         }
    //     }

    //     if (geometries.size() > 0)
    //     {
    //         writes.push_back(vk::WriteDescriptorSet(
    //             *descSet, DescBinding::eVertexBuffers, 0, vertBufs.size(),
    //             vk::DescriptorType::eStorageBuffer,
    //             nullptr,
    //             vertBufs.data()
    //         ));
    //         writes.push_back(vk::WriteDescriptorSet(
    //             *descSet, DescBinding::eIndexBuffers, 0, indexBufs.size(),
    //             vk::DescriptorType::eStorageBuffer,
    //             nullptr,
    //             indexBufs.data()
    //         ));
    //     }
    // }
}
