#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "utils/PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "drawable/DrawableInstanced.h"

#include "PipelineRegistry.h"
#include "DeferredRenderConfig.h"



namespace trc::internal
{

namespace
{
    enum DrawablePipelineFeatureFlagBits : ui32
    {
        eNone = 0,
        eAnimated = 1 << 0,
        ePickable = 1 << 1,
    };
}

auto makeAllDrawablePipelines() -> std::vector<Pipeline>;
auto makeDrawableDeferredPipeline(ui32 featureFlags,
                                  const Instance& instance,
                                  const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableTransparentPipeline(ui32 featureFlags,
                                     const Instance& instance,
                                     const DeferredRenderConfig& config) -> Pipeline;
auto makeInstancedDrawableDeferredPipeline(const Instance& instance,
                                           const DeferredRenderConfig& config) -> Pipeline;
auto makeDrawableShadowPipeline(const Instance& instance,
                                const DeferredRenderConfig& config) -> Pipeline;
auto makeInstancedDrawableShadowPipeline(const Instance& instance,
                                         const DeferredRenderConfig& config) -> Pipeline;

using Flags = DrawablePipelineFeatureFlagBits;

auto getDrawableDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableDeferredPipeline(0, instance, config);
        }
    );

    return id;
}

auto getDrawableDeferredAnimatedPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableDeferredPipeline(Flags::eAnimated, instance, config);
        }
    );

    return id;
}

auto getDrawableDeferredPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableDeferredPipeline(Flags::ePickable, instance, config);
        }
    );

    return id;
}

auto getDrawableDeferredAnimatedAndPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableDeferredPipeline(Flags::eAnimated | Flags::ePickable, instance, config);
        }
    );

    return id;
}

auto getDrawableTransparentDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableTransparentPipeline(Flags::eNone, instance, config);
        }
    );

    return id;
}

auto getDrawableTransparentDeferredAnimatedPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableTransparentPipeline(Flags::eAnimated, instance, config);
        }
    );

    return id;
}

auto getDrawableTransparentDeferredPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config) {
            return makeDrawableTransparentPipeline(Flags::ePickable, instance, config);
        }
    );

    return id;
}

auto getDrawableTransparentDeferredAnimatedAndPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        [](const Instance& instance, const auto& config)
        {
            return makeDrawableTransparentPipeline(
                Flags::eAnimated | Flags::ePickable,
                instance, config
            );
        }
    );

    return id;
}

auto getDrawableShadowPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        makeDrawableShadowPipeline
    );

    return id;
}

auto getDrawableInstancedDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        makeInstancedDrawableDeferredPipeline
    );

    return id;
}

auto getDrawableInstancedShadowPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        makeInstancedDrawableShadowPipeline
    );

    return id;
}



auto makeDrawableDeferredPipeline(
    ui32 featureFlags,
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
    bool32 vertConst = (featureFlags & DrawablePipelineFeatureFlagBits::eAnimated) != 0;
    bool32 fragConst = (featureFlags & DrawablePipelineFeatureFlagBits::ePickable) != 0;
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
    ui32 featureFlags,
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
    bool32 vertConst = (featureFlags & DrawablePipelineFeatureFlagBits::eAnimated) != 0;
    bool32 fragConst = (featureFlags & DrawablePipelineFeatureFlagBits::ePickable) != 0;
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
            *vkb::VulkanBase::getDevice(),
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

auto makeInstancedDrawableDeferredPipeline(
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
        },
        std::vector<vk::PushConstantRange>{
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eFragment,
                84,
                sizeof(ui32)  // Picking ID
            ),
        }
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/instanced.vert.spv",
                               SHADER_DIR / "drawable/deferred.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // Vertex attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        // Per-instance attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(
                1, sizeof(DrawableInstanced::InstanceDescription), vk::VertexInputRate::eInstance
            ),
            {
                // Model matrix
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),
                // Material index
                vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32Uint, 64),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::gBuffer
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };

    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getDeferredPassDescriptorProvider());

    p.addDefaultPushConstantValue(84, NO_PICKABLE, vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeInstancedDrawableShadowPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>
        {
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>
        {
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(ui32)  // light index
            )
        }
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "drawable/shadow_instanced.vert.spv",
                               SHADER_DIR / "drawable/shadow.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // Default vertex attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        // Per-instance attributes
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(
                1, sizeof(DrawableInstanced::InstanceDescription), vk::VertexInputRate::eInstance
            ),
            {
                // Model matrix
                vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, 0),
                vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 16),
                vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, 32),
                vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 48),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))  // Dynamic state
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))     // Dynamic state
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*instance.getDevice(), *layout, config.getCompatibleShadowRenderPass(), 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getShadowDescriptorProvider());

    return p;
}

auto makeFinalLightingPipeline(
    const Instance& instance,
    const DeferredRenderConfig& config) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        instance.getDevice(),
        std::vector<vk::DescriptorSetLayout>
        {
            config.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            config.getAssets().getDescriptorSetProvider().getDescriptorSetLayout(),
            config.getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            config.getSceneDescriptorProvider().getDescriptorSetLayout(),
            config.getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    // Pipeline
    vkb::ShaderProgram program(instance.getDevice(),
                               SHADER_DIR / "final_lighting.vert.spv",
                               SHADER_DIR / "final_lighting.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec3), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
            }
        )
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, { 1, 1 }))
        .disableBlendAttachments(1)
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(
            *instance.getDevice(),
            *layout,
            *config.getDeferredRenderPass(), RenderPassDeferred::SubPasses::lighting
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, config.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, config.getAssets().getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, config.getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(3, config.getSceneDescriptorProvider());
    p.addStaticDescriptorSet(4, config.getShadowDescriptorProvider());

    return p;
}

auto getFinalLightingPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry<DeferredRenderConfig>::registerPipeline(
        makeFinalLightingPipeline
    );

    return id;
}

} // namespace trc::internal
