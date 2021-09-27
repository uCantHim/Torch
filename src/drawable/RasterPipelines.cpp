#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
#include "PipelineDefinitions.h"
#include "DeferredRenderConfig.h"
#include "AnimationEngine.h"



namespace trc
{

auto getPipeline(PipelineFeatureFlags featureFlags) -> Pipeline::ID
{
    if (featureFlags & PipelineFeatureFlagBits::eShadow)
    {
        assert(featureFlags == (featureFlags & PipelineFeatureFlagBits::eShadow));
        return getDrawableShadowPipeline();
    }

    if (featureFlags & PipelineFeatureFlagBits::eTransparent)
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated
            && featureFlags & PipelineFeatureFlagBits::ePickable)
        {
            return getDrawableTransparentDeferredAnimatedAndPickablePipeline();
        }
        else if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            return getDrawableTransparentDeferredAnimatedPipeline();
        }
        else if (featureFlags & PipelineFeatureFlagBits::ePickable) {
            return getDrawableTransparentDeferredPickablePipeline();
        }
        else {
            return getDrawableTransparentDeferredPipeline();
        }
    }
    else
    {
        if (featureFlags & PipelineFeatureFlagBits::eAnimated
            && featureFlags & PipelineFeatureFlagBits::ePickable)
        {
            return getDrawableDeferredAnimatedAndPickablePipeline();
        }
        else if (featureFlags & PipelineFeatureFlagBits::eAnimated) {
            return getDrawableDeferredAnimatedPipeline();
        }
        else if (featureFlags & PipelineFeatureFlagBits::ePickable) {
            return getDrawableDeferredPickablePipeline();
        }
        else {
            return getDrawableDeferredPipeline();
        }
    }
}



auto makeAllDrawablePipelines() -> std::vector<Pipeline>;
auto makeDrawableDeferredPipeline(PipelineFeatureFlags featureFlags,
                                  const Instance& instance,
                                  const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags,
                                     const Instance& instance,
                                     const DeferredRenderConfig& config) -> Pipeline;
auto makeInstancedDrawableDeferredPipeline(const Instance& instance,
                                           const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableShadowPipeline(const Instance& instance,
                                const DeferredRenderConfig& config) -> Pipeline;
auto makeInstancedDrawableShadowPipeline(const Instance& instance,
                                         const DeferredRenderConfig& config) -> Pipeline;
auto makeFinalLightingPipeline(const Instance& instance,
                               const DeferredRenderConfig& config) -> Pipeline;

using Flags = PipelineFeatureFlagBits;

auto _makeDrawDef = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline({}, instance, config);
};
auto _makeDrawDefAnim = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline(Flags::eAnimated, instance, config);
};
auto _makeDrawDefPick = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline(Flags::ePickable, instance, config);
};
auto _makeDrawDefAnimPick = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline(Flags::ePickable | Flags::eAnimated, instance, config);
};

auto _makeTransDef = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline({}, instance, config);
};
auto _makeTransDefAnim = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline(Flags::eAnimated, instance, config);
};
auto _makeTransDefPick = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline(Flags::ePickable, instance, config);
};
auto _makeTransDefAnimPick = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline(Flags::eAnimated | Flags::ePickable, instance, config);
};

PIPELINE_GETTER_FUNC(getDrawableDeferredPipeline, _makeDrawDef, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableDeferredAnimatedPipeline, _makeDrawDefAnim, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableDeferredPickablePipeline, _makeDrawDefPick, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableDeferredAnimatedAndPickablePipeline, _makeDrawDefAnimPick, DeferredRenderConfig)

PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredPipeline, _makeTransDef, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredAnimatedPipeline, _makeTransDefAnim, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredPickablePipeline, _makeTransDefPick, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredAnimatedAndPickablePipeline, _makeTransDefAnimPick, DeferredRenderConfig)

PIPELINE_GETTER_FUNC(getDrawableShadowPipeline, makeDrawableShadowPipeline, DeferredRenderConfig);
PIPELINE_GETTER_FUNC(getDrawableInstancedDeferredPipeline, makeInstancedDrawableDeferredPipeline, DeferredRenderConfig);
PIPELINE_GETTER_FUNC(getDrawableInstancedShadowPipeline, makeInstancedDrawableShadowPipeline, DeferredRenderConfig);

PIPELINE_GETTER_FUNC(getFinalLightingPipeline, makeFinalLightingPipeline, DeferredRenderConfig);



auto makeDrawableDeferredPipeline(
    PipelineFeatureFlags featureFlags,
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout> {
            config.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            config.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
            config.getSceneDescriptorProvider().getDescriptorSetLayout(),
            config.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            config.getAnimationDataDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange> {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                // General drawable data
                sizeof(mat4)    // model matrix
                + sizeof(ui32)  // material index
                // Animation related data
                + sizeof(ui32)  // current animation index (UINT32_MAX if no animation)
                + sizeof(uvec2) // active keyframes
                + sizeof(float) // keyframe weight
            ),
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eFragment,
                84,
                sizeof(ui32)  // Picking ID
            ),
        }
    );

    // Pipeline
    bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);
    bool32 fragConst = !!(featureFlags & PipelineFeatureFlagBits::ePickable);
    vk::SpecializationMapEntry vertEntry(0, 0, sizeof(ui32));
    vk::SpecializationMapEntry fragEntry(1, 0, sizeof(ui32));
    vk::SpecializationInfo vertSpec(1, &vertEntry, sizeof(ui32) * 1, &vertConst);
    vk::SpecializationInfo fragSpec(1, &fragEntry, sizeof(ui32) * 1, &fragConst);

    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/deferred.vert.spv",
                               SHADER_DIR / "drawable/deferred.frag.spv");
    program.setVertexSpecializationConstants(&vertSpec);
    program.setFragmentSpecializationConstants(&fragSpec);

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *instance.getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::gBuffer
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(4, config.getAnimationDataDescriptorProvider());

    p.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(84, NO_PICKABLE,  vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeDrawableTransparentPipeline(
    PipelineFeatureFlags featureFlags,
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout> {
            config.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            config.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
            config.getSceneDescriptorProvider().getDescriptorSetLayout(),
            config.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            config.getAnimationDataDescriptorProvider().getDescriptorSetLayout(),
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange> {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                // General drawable data
                sizeof(mat4)    // model matrix
                + sizeof(ui32)  // material index
                // Animation related data
                + sizeof(ui32)  // current animation index (UINT32_MAX if no animation)
                + sizeof(uvec2) // active keyframes
                + sizeof(float) // keyframe weight
            ),
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eFragment,
                84,
                sizeof(ui32)  // Picking ID
            ),
        }
    );

    // Pipeline
    bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);
    bool32 fragConst = !!(featureFlags & PipelineFeatureFlagBits::ePickable);
    vk::SpecializationMapEntry vertEntry(0, 0, sizeof(ui32));
    vk::SpecializationMapEntry fragEntry(1, 0, sizeof(ui32));
    vk::SpecializationInfo vertSpec(1, &vertEntry, sizeof(ui32) * 1, &vertConst);
    vk::SpecializationInfo fragSpec(1, &fragEntry, sizeof(ui32) * 1, &fragConst);

    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/deferred.vert.spv",
                               SHADER_DIR / "drawable/transparent.frag.spv");
    program.setVertexSpecializationConstants(&vertSpec);
    program.setFragmentSpecializationConstants(&fragSpec);

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .setCullMode(vk::CullModeFlagBits::eNone) // Don't cull back faces because they're visible
        .disableDepthWrite()
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *instance.getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::transparency
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(4, config.getAnimationDataDescriptorProvider());
    p.addStaticDescriptorSet(5, config.getShadowDescriptorProvider());

    p.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(84, NO_PICKABLE,  vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeDrawableShadowPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>
        {
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
            config.getAnimationDataDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>
        {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(mat4)    // model matrix
                + sizeof(ui32)  // light index
                // Animation related data
                + sizeof(ui32)  // current animation index (UINT32_MAX if no animation)
                + sizeof(uvec2) // active keyframes
                + sizeof(float) // keyframe weight
            )
        }
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/shadow.vert.spv",
                               SHADER_DIR / "drawable/shadow.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))  // Dynamic state
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))     // Dynamic state
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*instance.getDevice(), *layout, config.getCompatibleShadowRenderPass(), 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getShadowDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAnimationDataDescriptorProvider());

    return p;
}

} // namespace trc
