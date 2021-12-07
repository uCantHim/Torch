#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
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

auto makeDrawablePoolInstancedPipeline(
    const Instance& instance,
    const DeferredRenderConfig& conf,
    PipelineFeatureFlags flags)
    -> Pipeline
{
    const bool isTransparent = !!(flags & PipelineFeatureFlagBits::eTransparent);

    vkb::ShaderProgram program(instance.getDevice(),
                               TRC_SHADER_DIR"/drawable/pool_instance.vert.spv",
                               isTransparent ? TRC_SHADER_DIR"/drawable/transparent.frag.spv"
                                             : TRC_SHADER_DIR"/drawable/deferred.frag.spv");

    auto layout = makePipelineLayout(instance.getDevice(),
        [&] {
            std::vector<vk::DescriptorSetLayout> layouts{
                conf.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
                conf.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
                conf.getSceneDescriptorProvider().getDescriptorSetLayout(),
                conf.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
                conf.getAnimationDataDescriptorProvider().getDescriptorSetLayout(),
            };
            if (flags & PipelineFeatureFlagBits::eTransparent) {
                layouts.emplace_back(conf.getShadowDescriptorProvider().getDescriptorSetLayout());
            }

            return layouts;
        }(),
        {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32))
        }
    );
    layout.addStaticDescriptorSet(0, conf.getGlobalDataDescriptorProvider());
    layout.addStaticDescriptorSet(1, conf.getAssets().getDescriptorSetProvider());
    layout.addStaticDescriptorSet(2, conf.getSceneDescriptorProvider());
    layout.addStaticDescriptorSet(3, conf.getDeferredPassDescriptorProvider());
    layout.addStaticDescriptorSet(4, conf.getAnimationDataDescriptorProvider());
    if (flags & PipelineFeatureFlagBits::eTransparent) {
        layout.addStaticDescriptorSet(5, conf.getShadowDescriptorProvider());
    }

    auto builder = GraphicsPipelineBuilder::create()
        .setProgram(program)
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
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .disableBlendAttachments(3);

    if (isTransparent)
    {
        builder.setCullMode(vk::CullModeFlagBits::eNone);
        builder.disableDepthWrite();
    }

    auto pipeline = builder.build(
            *instance.getDevice(),
            *layout,
            *conf.getDeferredRenderPass(),
            isTransparent ? RenderPassDeferred::SubPasses::transparency
                          : RenderPassDeferred::SubPasses::gBuffer
        );

    Pipeline p(std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics);

    return p;
}

auto makeDrawablePoolInstancedShadowPipeline(const Instance& instance, const DeferredRenderConfig& conf)
    -> Pipeline
{
    vkb::ShaderProgram program(instance.getDevice(),
                               TRC_SHADER_DIR"/drawable/pool_instance_shadow.vert.spv",
                               TRC_SHADER_DIR"/empty.frag.spv");

    auto layout = makePipelineLayout(instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>{
            conf.getShadowDescriptorProvider().getDescriptorSetLayout(),
            conf.getAnimationDataDescriptorProvider().getDescriptorSetLayout(),
        },
        {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32)) // light index
        }
    );
    layout.addStaticDescriptorSet(0, conf.getShadowDescriptorProvider());
    layout.addStaticDescriptorSet(1, conf.getAnimationDataDescriptorProvider());

    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
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
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .disableBlendAttachments(1)
        .build(*instance.getDevice(), *layout, conf.getCompatibleShadowRenderPass(), 0);

    Pipeline p(std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics);

    return p;
}

auto _makePoolInst = [](const Instance& instance, const auto& config) {
    return makeDrawablePoolInstancedPipeline(instance, config, {});
};
auto _makePoolInstTrans = [](const Instance& instance, const auto& config) {
    return makeDrawablePoolInstancedPipeline(instance, config, PipelineFeatureFlagBits::eTransparent);
};

PIPELINE_GETTER_FUNC(getInstancedPoolPipeline, _makePoolInst, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getInstancedPoolTransparentPipeline, _makePoolInstTrans, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getInstancedPoolShadowPipeline,
                     makeDrawablePoolInstancedShadowPipeline,
                     DeferredRenderConfig)



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



auto makeAllDrawablePipelines() -> std::vector<Pipeline>;
auto makeDrawableDeferredPipeline(PipelineFeatureFlags featureFlags,
                                  const Instance& instance,
                                  const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableTransparentPipeline(PipelineFeatureFlags featureFlags,
                                     const Instance& instance,
                                     const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableShadowPipeline(const Instance& instance,
                                const DeferredRenderConfig& config) -> Pipeline;

using Flags = PipelineFeatureFlagBits;

auto _makeDrawDef = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline({}, instance, config);
};
auto _makeDrawDefAnim = [](const Instance& instance, const auto& config) {
    return makeDrawableDeferredPipeline(Flags::eAnimated, instance, config);
};

auto _makeTransDef = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline({}, instance, config);
};
auto _makeTransDefAnim = [](const Instance& instance, const auto& config) {
    return makeDrawableTransparentPipeline(Flags::eAnimated, instance, config);
};

PIPELINE_GETTER_FUNC(getDrawableDeferredPipeline, _makeDrawDef, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableDeferredAnimatedPipeline, _makeDrawDefAnim, DeferredRenderConfig)

PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredPipeline, _makeTransDef, DeferredRenderConfig)
PIPELINE_GETTER_FUNC(getDrawableTransparentDeferredAnimatedPipeline, _makeTransDefAnim, DeferredRenderConfig)

PIPELINE_GETTER_FUNC(getDrawableShadowPipeline, makeDrawableShadowPipeline, DeferredRenderConfig);



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
        }
    );
    layout.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    layout.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    layout.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    layout.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());
    layout.addStaticDescriptorSet(4, config.getAnimationDataDescriptorProvider());

    layout.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);

    // Pipeline
    bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);
    vk::SpecializationMapEntry vertEntry(0, 0, sizeof(ui32));
    vk::SpecializationInfo vertSpec(1, &vertEntry, sizeof(ui32) * 1, &vertConst);

    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/deferred.vert.spv",
                               SHADER_DIR / "drawable/deferred.frag.spv");
    program.setVertexSpecializationConstants(&vertSpec);

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
        }
    );
    layout.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    layout.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    layout.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    layout.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());
    layout.addStaticDescriptorSet(4, config.getAnimationDataDescriptorProvider());
    layout.addStaticDescriptorSet(5, config.getShadowDescriptorProvider());

    layout.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    layout.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);

    // Pipeline
    bool32 vertConst = !!(featureFlags & PipelineFeatureFlagBits::eAnimated);
    vk::SpecializationMapEntry vertEntry(0, 0, sizeof(ui32));
    vk::SpecializationInfo vertSpec(1, &vertEntry, sizeof(ui32) * 1, &vertConst);

    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/deferred.vert.spv",
                               SHADER_DIR / "drawable/transparent.frag.spv");
    program.setVertexSpecializationConstants(&vertSpec);

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
    layout.addStaticDescriptorSet(0, config.getShadowDescriptorProvider());
    layout.addStaticDescriptorSet(1, config.getAnimationDataDescriptorProvider());

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/shadow.vert.spv",
                               SHADER_DIR / "empty.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))  // Dynamic state
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))     // Dynamic state
#ifdef TRC_FLIP_Y_PROJECTION
        .setFrontFace(vk::FrontFace::eClockwise)
#endif
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*instance.getDevice(), *layout, config.getCompatibleShadowRenderPass(), 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };

    return p;
}

} // namespace trc
