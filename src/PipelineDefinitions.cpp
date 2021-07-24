#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "utils/PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "drawable/DrawableInstanced.h"
#include "Scene.h"
#include "Renderer.h"
#include "PipelineRegistry.h"



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
auto makeDrawableDeferredPipeline(ui32 featureFlags, vk::RenderPass deferredPass) -> Pipeline;
auto makeDrawableTransparentPipeline(ui32 featureFlags, vk::RenderPass deferredPass) -> Pipeline;
auto makeInstancedDrawableDeferredPipeline(vk::RenderPass deferredPass) -> Pipeline;
auto makeDrawableShadowPipeline(RenderPassShadow& renderPass) -> Pipeline;
auto makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass) -> Pipeline;

/**
 * Stores all dynamically created pipeline IDs
 */
static vk::UniqueRenderPass dummyDeferredPass;
vkb::StaticInit _init{
    [] { dummyDeferredPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain()); },
    [] { dummyDeferredPass.reset(); }
};

using Flags = DrawablePipelineFeatureFlagBits;

auto getDrawableDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableDeferredPipeline(0, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableDeferredAnimatedPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableDeferredPipeline(Flags::eAnimated, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableDeferredPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableDeferredPipeline(Flags::ePickable, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableDeferredAnimatedAndPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableDeferredPipeline(Flags::eAnimated | Flags::ePickable, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableTransparentDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableTransparentPipeline(Flags::eNone, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableTransparentDeferredAnimatedPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableTransparentPipeline(Flags::eAnimated, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableTransparentDeferredPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableTransparentPipeline(Flags::ePickable, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableTransparentDeferredAnimatedAndPickablePipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeDrawableTransparentPipeline(Flags::eAnimated | Flags::ePickable, *dummyDeferredPass);
    });

    return id;
}

auto getDrawableShadowPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        RenderPassShadow shadowPass{ { 1, 1 } };
        return makeDrawableShadowPipeline(shadowPass);
    });

    return id;
}

auto getDrawableInstancedDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        return makeInstancedDrawableDeferredPipeline(*dummyDeferredPass);
    });

    return id;
}

auto getDrawableInstancedShadowPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        RenderPassShadow shadowPass{ { 1, 1 } };
        return makeInstancedDrawableShadowPipeline(shadowPass);
    });

    return id;
}


auto makeAllDrawablePipelines() -> std::vector<::trc::Pipeline>
{
    using Flags = DrawablePipelineFeatureFlagBits;
    RenderPassShadow shadowPass({ 1, 1 });
    auto deferredPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain());

    std::vector<Pipeline> result;

    // Default deferred
    result.emplace_back(makeDrawableDeferredPipeline(Flags::eNone, *deferredPass));
    result.emplace_back(makeDrawableDeferredPipeline(Flags::eAnimated, *deferredPass));
    result.emplace_back(makeDrawableDeferredPipeline(Flags::ePickable, *deferredPass));
    result.emplace_back(makeDrawableDeferredPipeline(Flags::eAnimated | Flags::ePickable, *deferredPass));

    // Transparent
    result.emplace_back(makeDrawableTransparentPipeline(Flags::eNone, *deferredPass));
    result.emplace_back(makeDrawableTransparentPipeline(Flags::ePickable, *deferredPass));
    result.emplace_back(makeDrawableTransparentPipeline(Flags::eAnimated, *deferredPass));
    result.emplace_back(makeDrawableTransparentPipeline(Flags::ePickable | Flags::eAnimated, *deferredPass));

    // Shadow
    result.emplace_back(makeDrawableShadowPipeline(shadowPass));

    // Instanced
    result.emplace_back(makeInstancedDrawableDeferredPipeline(*deferredPass));
    result.emplace_back(makeInstancedDrawableShadowPipeline(shadowPass));

    return result;
}

auto makeDrawableDeferredPipeline(
    ui32 featureFlags,
    vk::RenderPass deferredPass) -> Pipeline
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            Renderer::getSceneDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            Animation::getDescriptorProvider().getDescriptorSetLayout(),
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

    vkb::ShaderProgram program(SHADER_DIR / "drawable/deferred.vert.spv",
                               SHADER_DIR / "drawable/deferred.frag.spv");
    program.setVertexSpecializationConstants(&vertSpec);
    program.setFragmentSpecializationConstants(&fragSpec);

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
            makeVertexAttributeDescriptions()
        )
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            deferredPass, DeferredSubPasses::gBufferPass
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Renderer::getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, Renderer::getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(4, Animation::getDescriptorProvider());

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
    vk::RenderPass deferredPass) -> Pipeline
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            Renderer::getSceneDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            Animation::getDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getShadowDescriptorProvider().getDescriptorSetLayout(),
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

    vkb::ShaderProgram program(SHADER_DIR / "drawable/deferred.vert.spv",
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
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addDynamicState(vk::DynamicState::eViewport)
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            deferredPass, DeferredSubPasses::transparencyPass
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Renderer::getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, Renderer::getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(4, Animation::getDescriptorProvider());
    p.addStaticDescriptorSet(5, Renderer::getShadowDescriptorProvider());

    p.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(84, NO_PICKABLE,  vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeDrawableShadowPipeline(RenderPassShadow& renderPass) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            Renderer::getShadowDescriptorProvider().getDescriptorSetLayout(),
            Animation::getDescriptorProvider().getDescriptorSetLayout(),
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
    vkb::ShaderProgram program(SHADER_DIR / "drawable/shadow.vert.spv",
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
        .build(*vkb::VulkanBase::getDevice(), *layout, *renderPass, 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getShadowDescriptorProvider());
    p.addStaticDescriptorSet(1, Animation::getDescriptorProvider());

    return p;
}

auto makeInstancedDrawableDeferredPipeline(vk::RenderPass deferredPass) -> Pipeline
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            Renderer::getSceneDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
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
    vkb::ShaderProgram program(SHADER_DIR / "drawable/instanced.vert.spv",
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
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .disableBlendAttachments(3)
        .addDynamicState(vk::DynamicState::eViewport)
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            deferredPass, DeferredSubPasses::gBufferPass
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };

    p.addStaticDescriptorSet(0, Renderer::getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Renderer::getSceneDescriptorProvider());
    p.addStaticDescriptorSet(3, Renderer::getDeferredPassDescriptorProvider());

    p.addDefaultPushConstantValue(84, NO_PICKABLE, vk::ShaderStageFlagBits::eFragment);

    return p;
}

auto makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass) -> Pipeline
{
    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            Renderer::getShadowDescriptorProvider().getDescriptorSetLayout(),
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
    vkb::ShaderProgram program(SHADER_DIR / "drawable/shadow_instanced.vert.spv",
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
        .build(*vkb::VulkanBase::getDevice(), *layout, *renderPass, 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getShadowDescriptorProvider());

    return p;
}

auto makeFinalLightingPipeline(vk::RenderPass deferredPass) -> Pipeline
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            Renderer::getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getSceneDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getShadowDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    // Pipeline
    vkb::ShaderProgram program(SHADER_DIR / "final_lighting.vert.spv",
                               SHADER_DIR / "final_lighting.frag.spv");

    vk::UniquePipeline pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(vec3), vk::VertexInputRate::eVertex),
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
            }
        )
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .disableBlendAttachments(1)
        .addDynamicState(vk::DynamicState::eViewport)
        .build(
            *vkb::getDevice(),
            *layout,
            deferredPass, DeferredSubPasses::lightingPass
        );

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Renderer::getDeferredPassDescriptorProvider());
    p.addStaticDescriptorSet(3, Renderer::getSceneDescriptorProvider());
    p.addStaticDescriptorSet(4, Renderer::getShadowDescriptorProvider());

    return p;
}

auto getFinalLightingPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        auto renderPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain());
        return makeFinalLightingPipeline(*renderPass);
    });

    return id;
}

} // namespace trc::internal
