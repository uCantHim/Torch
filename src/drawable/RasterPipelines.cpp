#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "PipelineDefinitions.h"
#include "TorchRenderConfig.h"
#include "AnimationEngine.h"



namespace trc
{

auto makeVertexAttributeDescriptions(PipelineFeatureFlags flags)
    -> std::vector<vk::VertexInputAttributeDescription>
{
    if (flags & PipelineFeatureFlagBits::eAnimated)
    {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat,    0),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat,    12),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat,       24),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat,    32),
            vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32B32A32Uint,   44),
            vk::VertexInputAttributeDescription(5, 0, vk::Format::eR32G32B32A32Sfloat, 60),
        };
    }
    else {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat,    0),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat,    12),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat,       24),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat,    32),
        };
    }
}



auto makeDrawableDeferredPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;
auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;
auto makeDrawableShadowPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;

auto getPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    // Shadow
    if (featureFlags & PipelineFeatureFlagBits::eShadow)
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            static auto p = makeDrawableShadowPipeline(featureFlags);
            return p;
        }
        else {
            static auto p = makeDrawableShadowPipeline(featureFlags);
            return p;
        }
    }

    // Opaque or transparent
    if (featureFlags & PipelineFeatureFlagBits::eTransparent)
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            static auto p = makeDrawableTransparentPipeline(featureFlags);
            return p;
        }
        else {
            static auto p = makeDrawableTransparentPipeline(featureFlags);
            return p;
        }
    }
    else
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            static auto p = makeDrawableDeferredPipeline(featureFlags);
            return p;
        }
        else {
            static auto p = makeDrawableDeferredPipeline(featureFlags);
            return p;
        }
    }
}



struct DrawablePushConstants
{
    mat4 model{ 1.0f };
    ui32 materialIndex{ 0u };

    ui32 animationIndex{ NO_ANIMATION };
    uvec2 keyframes{ 0, 0 };
    float keyframeWeight{ 0.0f };
};

auto makeDrawableDeferredPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    const bool isAnimated = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) },
            DrawablePushConstants{}
        )
        .registerLayout<TorchRenderConfig>();

    const auto vertShader = isAnimated ? "drawable/deferred_animated.vert.spv"
                                       : "drawable/deferred.vert.spv";

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / vertShader),
                    vkb::readFile(SHADER_DIR / "drawable/deferred.frag.spv"))
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(MeshVertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions(featureFlags)
        )
        .disableBlendAttachments(3)
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );
}

auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    const bool isAnimated = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) },
            DrawablePushConstants{}
        )
        .registerLayout<TorchRenderConfig>();

    const auto vertShader = isAnimated ? "drawable/deferred_animated.vert.spv"
                                       : "drawable/deferred.vert.spv";

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / vertShader),
                    vkb::readFile(SHADER_DIR / "drawable/transparent.frag.spv"))
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(MeshVertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions(featureFlags)
        )
        .setCullMode(vk::CullModeFlagBits::eNone) // Don't cull back faces because they're visible
        .disableDepthWrite()
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::TRANSPARENT_G_BUFFER_PASS }
        );
}

auto makeDrawableShadowPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    const bool isAnimated = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) },
            DrawablePushConstants{}
        )
        .registerLayout<TorchRenderConfig>();

    const auto vertShader = isAnimated ? "drawable/shadow_animated.vert.spv"
                                       : "drawable/shadow.vert.spv";

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / vertShader),
                    vkb::readFile(SHADER_DIR / "empty.frag.spv"))
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(MeshVertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions(featureFlags)
        )
        .setCullMode(vk::CullModeFlagBits::eFront)
        .setRasterization(
            vk::PipelineRasterizationStateCreateInfo(DEFAULT_RASTERIZATION)
            .setDepthBiasConstantFactor(4.0f)
        )
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::SHADOW_PASS }
        );
}

} // namespace trc
