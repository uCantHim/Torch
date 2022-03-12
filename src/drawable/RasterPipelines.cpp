#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "PipelineDefinitions.h"
#include "TorchRenderConfig.h"
#include "AnimationEngine.h"



namespace trc
{

auto getDrawableDeferredPipeline() -> Pipeline::ID;
auto getDrawableDeferredAnimatedPipeline() -> Pipeline::ID;

auto getDrawableTransparentDeferredPipeline() -> Pipeline::ID;
auto getDrawableTransparentDeferredAnimatedPipeline() -> Pipeline::ID;

auto getDrawableShadowPipeline() -> Pipeline::ID;



auto getPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    if (featureFlags & PipelineFeatureFlagBits::eShadow)
    {
        assert(featureFlags == (featureFlags & PipelineFeatureFlagBits::eShadow));
        return getDrawableShadowPipeline();
    }

    if (featureFlags & PipelineFeatureFlagBits::eTransparent)
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            return getDrawableTransparentDeferredAnimatedPipeline();
        }
        else {
            return getDrawableTransparentDeferredPipeline();
        }
    }
    else
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            return getDrawableDeferredAnimatedPipeline();
        }
        else {
            return getDrawableDeferredPipeline();
        }
    }
}



auto makeDrawableDeferredPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;
auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID;
auto makeDrawableShadowPipeline() -> Pipeline::ID;

using Flags = PipelineFeatureFlagBits;

auto _makeDrawDef = []() {
    return makeDrawableDeferredPipeline({});
};
auto _makeDrawDefAnim = []() {
    return makeDrawableDeferredPipeline(Flags::eAnimated);
};
auto _makeTransDef = []() {
    return makeDrawableTransparentPipeline({});
};
auto _makeTransDefAnim = []() {
    return makeDrawableTransparentPipeline(Flags::eAnimated);
};

PIPELINE_GETTER_FUNC(getDrawableDeferredPipeline, _makeDrawDef)
PIPELINE_GETTER_FUNC(getDrawableDeferredAnimatedPipeline, _makeDrawDefAnim)

PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredPipeline, _makeTransDef)
PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredAnimatedPipeline, _makeTransDefAnim)

PIPELINE_GETTER_FUNC(getDrawableShadowPipeline, makeDrawableShadowPipeline)



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

    const bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / "drawable/deferred.vert.spv"),
                    vkb::readFile(SHADER_DIR / "drawable/deferred.frag.spv"))
        .setVertexSpecializationConstant(0, vertConst)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .disableBlendAttachments(3)
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );
}

auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
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

    const bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / "drawable/deferred.vert.spv"),
                    vkb::readFile(SHADER_DIR / "drawable/transparent.frag.spv"))
        .setVertexSpecializationConstant(0, vertConst)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .setCullMode(vk::CullModeFlagBits::eNone) // Don't cull back faces because they're visible
        .disableDepthWrite()
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::TRANSPARENT_G_BUFFER_PASS }
        );
}

auto makeDrawableShadowPipeline() -> Pipeline::ID
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) }
        )
        .registerLayout<TorchRenderConfig>();

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / "drawable/shadow.vert.spv"),
                    vkb::readFile(SHADER_DIR / "empty.frag.spv"))
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
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
