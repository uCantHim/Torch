#include "trc/assets/AssetRegistry.h"

#include "trc/base/ImageUtils.h"

#include "trc/assets/AnimationRegistry.h"
#include "trc/assets/GeometryRegistry.h"
#include "trc/assets/MaterialRegistry.h"
#include "trc/assets/RigRegistry.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/assets/TextureRegistry.h"
#include "trc/core/Instance.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"
#include "trc/text/Font.h"



void trc::AssetRegistry::AssetModuleUpdatePass::update(
    vk::CommandBuffer cmdBuf,
    FrameRenderState& frameState)
{
    registry->update(cmdBuf, frameState);
}



trc::AssetRegistry::AssetRegistry(
    const Instance& instance,
    const AssetRegistryCreateInfo& info)
    :
    device(instance.getDevice()),
    config(addDefaultValues(info)),
    descSet(nullptr)
{
    auto builder = SharedDescriptorSet::build();

    // Add modules in the order in which they should be destroyed
    addModule<Material>(MaterialRegistryCreateInfo{
        .device=instance.getDevice(),
        .descriptorBuilder=builder,
        .descriptorConfig=makeShaderDescriptorConfig()
    });
    addModule<Texture>(TextureRegistryCreateInfo{ instance.getDevice(), builder });
    addModule<Geometry>(GeometryRegistryCreateInfo{
        .instance            = instance,
        .descriptorBuilder   = builder,
        .geometryBufferUsage = config.geometryBufferUsage,
        .enableRayTracing    = config.enableRayTracing && instance.hasRayTracing(),
    });
    addModule<Rig>();
    addModule<Animation>(AnimationRegistryCreateInfo{ instance.getDevice(), builder });
    addModule<Font>(FontRegistryCreateInfo{ instance.getDevice() });

    // Create descriptors
    descSet = builder.build(device);

    // Add default assets
    //add<Material>(std::make_unique<InMemorySource<Material>>(MaterialData{ .doPerformLighting=false }));
    add<Texture>(std::make_unique<InMemorySource<Texture>>(
        TextureData{ { 1, 1 }, makeSinglePixelImageData(vec4(1.0f)).pixels }
    ));
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

void trc::AssetRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState)
{
    modules.update(cmdBuf, frameState);
    descSet->update(device);
}
