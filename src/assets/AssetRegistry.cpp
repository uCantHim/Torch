#include "assets/AssetRegistry.h"

#include <vkb/ImageUtils.h>

#include "core/Instance.h"
#include "ray_tracing/RayPipelineBuilder.h"
#include "assets/GeometryRegistry.h"
#include "assets/MaterialRegistry.h"
#include "assets/TextureRegistry.h"
#include "assets/RigRegistry.h"
#include "assets/AnimationRegistry.h"



void trc::AssetRegistry::AssetModuleUpdatePass::update(vk::CommandBuffer cmdBuf)
{
    registry->update(cmdBuf);
}



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

    // Update modules once
    device.executeCommandsSynchronously(vkb::QueueType::transfer, [this](auto cmdBuf) {
        updateRenderPass->update(cmdBuf);
        descSet->update(device);
    });
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Geometry>> src) -> LocalID<Geometry>
{
    return modules.get<AssetRegistryModule<Geometry>>().add(std::move(src));
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Material>> src) -> LocalID<Material>
{
    return modules.get<AssetRegistryModule<Material>>().add(std::move(src));
}

auto trc::AssetRegistry::add(u_ptr<AssetSource<Texture>> src) -> LocalID<Texture>
{
    return modules.get<AssetRegistryModule<Texture>>().add(std::move(src));
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

auto trc::AssetRegistry::getUpdatePass() -> UpdatePass&
{
    return *updateRenderPass;
}

auto trc::AssetRegistry::getDescriptorSetProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return descSet->getProvider();
}

auto trc::AssetRegistry::addDefaultValues(AssetRegistryCreateInfo info)
    -> AssetRegistryCreateInfo
{
    info.materialDescriptorStages |= vk::ShaderStageFlagBits::eFragment
                                       | vk::ShaderStageFlagBits::eCompute;
    info.textureDescriptorStages |= vk::ShaderStageFlagBits::eFragment
                                      | vk::ShaderStageFlagBits::eCompute;

    if (info.enableRayTracing)
    {
        info.geometryBufferUsage |=
            vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress
            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

        info.materialDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
        info.textureDescriptorStages  |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
        info.geometryDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
    }

    return info;
}

void trc::AssetRegistry::update(vk::CommandBuffer cmdBuf)
{
    modules.foreach([cmdBuf](AssetRegistryModuleInterface& mod)
    {
        mod.update(cmdBuf);
    });
    descSet->update(device);
}
