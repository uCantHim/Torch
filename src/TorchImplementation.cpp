#include "trc/TorchImplementation.h"

#include "trc/assets/AnimationRegistry.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/assets/GeometryRegistry.h"
#include "trc/assets/MaterialRegistry.h"
#include "trc/assets/RigRegistry.h"
#include "trc/assets/TextureRegistry.h"
#include "trc/base/ImageUtils.h"
#include "trc/core/Instance.h"
#include "trc/ray_tracing/RayPipelineBuilder.h" // For ALL_RAY_PIPELINE_STAGE_FLAGS
#include "trc/text/Font.h"



namespace trc::impl
{

struct AssetRegistryCreateInfo
{
    vk::BufferUsageFlags geometryBufferUsage{};

    vk::ShaderStageFlags materialDescriptorStages{};
    vk::ShaderStageFlags textureDescriptorStages{};
    vk::ShaderStageFlags geometryDescriptorStages{};
};

auto makeDefaultDescriptorUsageSettings(const bool enableRayTracing)
    -> AssetRegistryCreateInfo
{
    AssetRegistryCreateInfo info{
        .materialDescriptorStages = vk::ShaderStageFlagBits::eFragment
                                           | vk::ShaderStageFlagBits::eCompute,
        .textureDescriptorStages = vk::ShaderStageFlagBits::eFragment
                                          | vk::ShaderStageFlagBits::eCompute,
    };

    if (enableRayTracing)
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

auto makeDefaultAssetModules(
    const Instance& instance,
    AssetRegistry& registry,
    const AssetDescriptorCreateInfo& descriptorCreateInfo) -> AssetDescriptor
{
    const Device& device = instance.getDevice();
    const auto config = makeDefaultDescriptorUsageSettings(instance.hasRayTracing());
    AssetDescriptor desc(device, descriptorCreateInfo);

    // Add modules in the order in which they should be destroyed
    registry.addModule<Material>(std::make_unique<MaterialRegistry>());
    registry.addModule<Texture>(std::make_unique<TextureRegistry>(
        TextureRegistryCreateInfo{
            device,
            desc.getBinding(AssetDescriptorBinding::eTextureSamplers)
        }
    ));
    registry.addModule<Geometry>(std::make_unique<GeometryRegistry>(
        GeometryRegistryCreateInfo{
            .instance            = instance,
            .indexDescriptorBinding = desc.getBinding(AssetDescriptorBinding::eGeometryIndexBuffers),
            .vertexDescriptorBinding = desc.getBinding(AssetDescriptorBinding::eGeometryVertexBuffers),
            .geometryBufferUsage = config.geometryBufferUsage,
            .enableRayTracing    = instance.hasRayTracing(),
        }
    ));
    registry.addModule<Rig>(std::make_unique<RigRegistry>());
    registry.addModule<Animation>(std::make_unique<AnimationRegistry>(
        AnimationRegistryCreateInfo{
            .device = device,
            .metadataDescBinding = desc.getBinding(AssetDescriptorBinding::eAnimationMetadata),
            .dataDescBinding = desc.getBinding(AssetDescriptorBinding::eAnimationData),
        }
    ));
    registry.addModule<Font>(std::make_unique<FontRegistry>(FontRegistryCreateInfo{ device }));

    // Add default assets
    registry.add<Texture>(std::make_unique<InMemorySource<Texture>>(
        TextureData{ { 1, 1 }, makeSinglePixelImageData(vec4(1.0f)).pixels }
    ));

    return desc;
}

} // namespace trc
