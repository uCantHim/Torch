#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "DrawableInstanced.h"
#include "Scene.h"
#include "Renderer.h"



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

    static const fs::path SHADER_DIR{ TRC_SHADER_DIR };
}



void makeAllDrawablePipelines(const Renderer& renderer)
{
    makeDrawableDeferredPipeline(renderer);
    makeDrawableDeferredAnimatedPipeline(renderer);
    makeDrawableDeferredPickablePipeline(renderer);
    makeDrawableDeferredAnimatedAndPickablePipeline(renderer);
    makeDrawableTransparentPipeline(
        Pipelines::eDrawableTransparentDeferred,
        DrawablePipelineFeatureFlagBits::eNone,
        renderer
    );
    makeDrawableTransparentPipeline(
        Pipelines::eDrawableTransparentDeferredPickable,
        DrawablePipelineFeatureFlagBits::ePickable,
        renderer
    );
    makeDrawableTransparentPipeline(
        Pipelines::eDrawableTransparentDeferredAnimated,
        DrawablePipelineFeatureFlagBits::eAnimated,
        renderer
    );
    makeDrawableTransparentPipeline(
        Pipelines::eDrawableTransparentDeferredAnimatedAndPickable,
        DrawablePipelineFeatureFlagBits::ePickable | DrawablePipelineFeatureFlagBits::eAnimated,
        renderer
    );
    makeInstancedDrawableDeferredPipeline(renderer);

    RenderPassShadow dummyPass({ 1, 1 }, mat4());
    makeDrawableShadowPipeline(dummyPass);
    makeInstancedDrawableShadowPipeline(dummyPass);
}

void makeDrawableDeferredPipeline(const Renderer& renderer)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferred,
        DrawablePipelineFeatureFlagBits::eNone,
        renderer
    );
}

void makeDrawableDeferredAnimatedPipeline(const Renderer& renderer)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredAnimated,
        DrawablePipelineFeatureFlagBits::eAnimated,
        renderer
    );
}

void makeDrawableDeferredPickablePipeline(const Renderer& renderer)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredPickable,
        DrawablePipelineFeatureFlagBits::ePickable,
        renderer
    );
}

void makeDrawableDeferredAnimatedAndPickablePipeline(const Renderer& renderer)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredAnimatedAndPickable,
        DrawablePipelineFeatureFlagBits::eAnimated | DrawablePipelineFeatureFlagBits::ePickable,
        renderer
    );
}

void _makeDrawableDeferredPipeline(
    ui32 pipelineIndex,
    ui32 featureFlags,
    const Renderer& renderer)
{
    const auto& renderPass = renderer.getDeferredRenderPass();
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            renderer.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
            renderPass.getDescriptorProvider().getDescriptorSetLayout(),
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
    ui32 constants[] {
        (featureFlags & DrawablePipelineFeatureFlagBits::eAnimated) != 0,
        (featureFlags & DrawablePipelineFeatureFlagBits::ePickable) != 0,
    };
    std::vector<vk::SpecializationMapEntry> specEntries{
        vk::SpecializationMapEntry(0, 0, sizeof(ui32)),
        vk::SpecializationMapEntry(1, sizeof(ui32), sizeof(ui32)),
    };
    vk::SpecializationInfo vertSpec(
        specEntries.size(), specEntries.data(), sizeof(ui32) * 2, constants);
    vk::SpecializationInfo fragSpec(
        specEntries.size(), specEntries.data(), sizeof(ui32) * 2, constants);

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
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, DeferredSubPasses::eGBufferPass
        );

    auto& p = makeGraphicsPipeline(pipelineIndex, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, renderer.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(3, renderPass.getDescriptorProvider());
    p.addStaticDescriptorSet(4, Animation::getDescriptorProvider());

    p.addDefaultPushConstantValue(0,  mat4(1.0f),   vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(64, 0u,           vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(68, NO_ANIMATION, vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(72, uvec2(0, 0),  vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(80, 0.0f,         vk::ShaderStageFlagBits::eVertex);
    p.addDefaultPushConstantValue(84, NO_PICKABLE,  vk::ShaderStageFlagBits::eFragment);
}

void makeDrawableTransparentPipeline(
    ui32 pipelineIndex,
    ui32 featureFlags,
    const Renderer& renderer)
{
    const auto& renderPass = renderer.getDeferredRenderPass();
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            renderer.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
            renderPass.getDescriptorProvider().getDescriptorSetLayout(),
            Animation::getDescriptorProvider().getDescriptorSetLayout(),
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
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
    ui32 constants[] {
        (featureFlags & DrawablePipelineFeatureFlagBits::eAnimated) != 0,
        (featureFlags & DrawablePipelineFeatureFlagBits::ePickable) != 0,
    };
    std::vector<vk::SpecializationMapEntry> specEntries{
        vk::SpecializationMapEntry(0, 0, sizeof(ui32)),
        vk::SpecializationMapEntry(1, sizeof(ui32), sizeof(ui32)),
    };
    vk::SpecializationInfo vertSpec(
        specEntries.size(), specEntries.data(), sizeof(ui32) * 2, constants);
    vk::SpecializationInfo fragSpec(
        specEntries.size(), specEntries.data(), sizeof(ui32) * 2, constants);

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
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, DeferredSubPasses::eTransparencyPass
        );

    auto& p = makeGraphicsPipeline(pipelineIndex, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, renderer.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(3, renderPass.getDescriptorProvider());
    p.addStaticDescriptorSet(4, Animation::getDescriptorProvider());
    p.addStaticDescriptorSet(5, ShadowDescriptor::getProvider());
}

void makeDrawableShadowPipeline(RenderPassShadow& renderPass)
{
    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
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

    auto& p = makeGraphicsPipeline(Pipelines::eDrawableShadow, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, ShadowDescriptor::getProvider());
    p.addStaticDescriptorSet(1, Animation::getDescriptorProvider());
}

void makeInstancedDrawableDeferredPipeline(const Renderer& renderer)
{
    const auto& renderPass = renderer.getDeferredRenderPass();
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            renderer.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
            renderPass.getDescriptorProvider().getDescriptorSetLayout(),
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
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect(vk::Rect2D({ 0, 0 }, extent))
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, DeferredSubPasses::eGBufferPass
        );

    auto& p = makeGraphicsPipeline(
        Pipelines::eDrawableInstancedDeferred,
        std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, renderer.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(3, renderPass.getDescriptorProvider());
}

void makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass)
{
    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
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

    auto& p = makeGraphicsPipeline(
        Pipelines::eDrawableInstancedShadow,
        std::move(layout), std::move(pipeline)
    );
    p.addStaticDescriptorSet(0, ShadowDescriptor::getProvider());
}

void makeFinalLightingPipeline(const Renderer& renderer)
{
    const auto& renderPass = renderer.getDeferredRenderPass();
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            renderer.getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            renderPass.getDescriptorProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
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
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(
            *vkb::getDevice(),
            *layout,
            *renderPass, DeferredSubPasses::eLightingPass
        );

    auto& p = makeGraphicsPipeline(Pipelines::eFinalLighting, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, renderer.getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, renderPass.getDescriptorProvider());
    p.addStaticDescriptorSet(3, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(4, ShadowDescriptor::getProvider());
}

} // namespace trc::internal
