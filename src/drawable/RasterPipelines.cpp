#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "PipelineDefinitions.h"
#include "DeferredRenderConfig.h"
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

auto makeDrawablePoolInstancedPipeline(PipelineFeatureFlags flags) -> Pipeline::ID
{
    const bool isTransparent = !!(flags & PipelineFeatureFlagBits::eTransparent);

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addDescriptorIf(isTransparent,
                         DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32) })
        .registerLayout<DeferredRenderConfig>();

    auto builder = buildGraphicsPipeline()
        .setProgram(vkb::readFile(TRC_SHADER_DIR"/drawable/pool_instance.vert.spv"),
                    vkb::readFile(isTransparent ? TRC_SHADER_DIR"/drawable/transparent.frag.spv"
                                                : TRC_SHADER_DIR"/drawable/deferred.frag.spv"))
        .addVertexInputBinding(
            { 0, sizeof(Vertex), vk::VertexInputRate::eVertex },
            makeVertexAttributeDescriptions()
        )
        .addVertexInputBinding(
            { 1, sizeof(mat4) + sizeof(uvec4), vk::VertexInputRate::eInstance },
            {
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),

                vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32G32B32A32Uint, 64),
            }
        )
        .disableBlendAttachments(3);

    if (isTransparent)
    {
        builder.setCullMode(vk::CullModeFlagBits::eNone);
        builder.disableDepthWrite();
    }

    return builder.registerPipeline<DeferredRenderConfig>(
        layout,
        isTransparent ? RenderPassName{ DeferredRenderConfig::TRANSPARENT_G_BUFFER_PASS }
                      : RenderPassName{ DeferredRenderConfig::OPAQUE_G_BUFFER_PASS }
    );
}

auto makeDrawablePoolInstancedShadowPipeline() -> Pipeline::ID
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32) })
        .registerLayout<DeferredRenderConfig>();

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(TRC_SHADER_DIR"/drawable/pool_instance_shadow.vert.spv"),
                    vkb::readFile(TRC_SHADER_DIR"/empty.frag.spv"))
        .addVertexInputBinding(
            { 0, sizeof(Vertex), vk::VertexInputRate::eVertex },
            makeVertexAttributeDescriptions()
        )
        .addVertexInputBinding(
            { 1, sizeof(mat4) + sizeof(uvec4), vk::VertexInputRate::eInstance },
            {
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),

                vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32G32B32A32Uint, 64),
            }
        )
        .disableBlendAttachments(1)
        .registerPipeline<DeferredRenderConfig>(
            layout,
            RenderPassName{ DeferredRenderConfig::SHADOW_PASS }
        );
}

auto _makePoolInst = []() {
    return makeDrawablePoolInstancedPipeline({});
};
auto _makePoolInstTrans = []() {
    return makeDrawablePoolInstancedPipeline(PipelineFeatureFlagBits::eTransparent);
};

PIPELINE_GETTER_FUNC(getInstancedPoolPipeline, _makePoolInst)
PIPELINE_GETTER_FUNC(getInstancedPoolTransparentPipeline, _makePoolInstTrans)
PIPELINE_GETTER_FUNC(getInstancedPoolShadowPipeline,
                     makeDrawablePoolInstancedShadowPipeline)



auto getPoolInstancePipeline(PipelineFeatureFlags flags) -> Pipeline::ID
{
    if (flags & PipelineFeatureFlagBits::eShadow)
    {
        return getInstancedPoolShadowPipeline();
    }
    else if (flags & PipelineFeatureFlagBits::eTransparent)
    {
        return getInstancedPoolTransparentPipeline();
    }
    else {
        return getInstancedPoolPipeline();
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
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) },
            DrawablePushConstants{}
        )
        .registerLayout<DeferredRenderConfig>();

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
        .registerPipeline<DeferredRenderConfig>(
            layout, RenderPassName{ DeferredRenderConfig::OPAQUE_G_BUFFER_PASS }
        );
}

auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) },
            DrawablePushConstants{}
        )
        .registerLayout<DeferredRenderConfig>();

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
        .registerPipeline<DeferredRenderConfig>(
            layout, RenderPassName{ DeferredRenderConfig::TRANSPARENT_G_BUFFER_PASS }
        );
}

auto makeDrawableShadowPipeline() -> Pipeline::ID
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ANIMATION_DESCRIPTOR }, true)
        .addPushConstantRange(
            { vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawablePushConstants) }
        )
        .registerLayout<DeferredRenderConfig>();

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / "drawable/shadow.vert.spv"),
                    vkb::readFile(SHADER_DIR / "empty.frag.spv"))
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .setCullMode(vk::CullModeFlagBits::eFront)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .registerPipeline<DeferredRenderConfig>(
            layout, RenderPassName{ DeferredRenderConfig::SHADOW_PASS }
        );
}

} // namespace trc
