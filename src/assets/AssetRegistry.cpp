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
    descSet(new SharedDescriptorSet),
    fontData(instance)
{
    auto builder = descSet->build();
    AssetRegistryModuleCreateInfo moduleCreateInfo{
        .device=instance.getDevice(),
        .layoutBuilder=&builder,
        .geometryBufferUsage=config.geometryBufferUsage,
        .enableRayTracing=config.enableRayTracing && instance.hasRayTracing(),
    };

    // Add modules in the order in which they should be destroyed
    modules.addModule<MaterialRegistry>(moduleCreateInfo);
    modules.addModule<TextureRegistry>(moduleCreateInfo);
    modules.addModule<GeometryRegistry>(moduleCreateInfo);
    modules.addModule<RigRegistry>(moduleCreateInfo);
    modules.addModule<AnimationRegistry>(moduleCreateInfo);

    // Create descriptors
    builder.build(device);

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
    return descSet->getProvider();
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

void trc::AssetRegistry::writeDescriptors()
{
    descSet->update(device);
}
