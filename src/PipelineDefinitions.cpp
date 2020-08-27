#include "PipelineDefinitions.h"

#include "PipelineBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "DrawableInstanced.h"
#include "Scene.h"



void trc::internal::makeAllDrawablePipelines(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet)
{
    makeDrawableDeferredPipeline(renderPass, generalDescriptorSet);
    makeDrawableDeferredAnimatedPipeline(renderPass, generalDescriptorSet);
    makeDrawableDeferredPickablePipeline(renderPass, generalDescriptorSet);
    makeDrawableDeferredAnimatedAndPickablePipeline(renderPass, generalDescriptorSet);

    makeInstancedDrawableDeferredPipeline(renderPass, generalDescriptorSet);

    RenderPassShadow dummyPass({ 1, 1 }, mat4());
    makeDrawableShadowPipeline(dummyPass);
    makeInstancedDrawableShadowPipeline(dummyPass);
}

void trc::internal::makeDrawableDeferredPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferred, renderPass, generalDescriptorSet,
        DrawablePipelineFeatureFlagBits::eNone
    );
}

void trc::internal::makeDrawableDeferredAnimatedPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredAnimated, renderPass, generalDescriptorSet,
        DrawablePipelineFeatureFlagBits::eAnimated
    );
}

void trc::internal::makeDrawableDeferredPickablePipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredPickable, renderPass, generalDescriptorSet,
        DrawablePipelineFeatureFlagBits::ePickable
    );
}

void trc::internal::makeDrawableDeferredAnimatedAndPickablePipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet)
{
    _makeDrawableDeferredPipeline(
        Pipelines::eDrawableDeferredAnimatedAndPickable, renderPass, generalDescriptorSet,
        DrawablePipelineFeatureFlagBits::eAnimated | DrawablePipelineFeatureFlagBits::ePickable
    );
}

void trc::internal::_makeDrawableDeferredPipeline(
    ui32 pipelineIndex,
    RenderPass& renderPass,
    const DescriptorProviderInterface& cameraDescriptorSet,
    ui32 featureFlags)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            cameraDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
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

    vkb::ShaderProgram program(TRC_SHADER_DIR"/drawable/deferred.vert.spv",
                               TRC_SHADER_DIR"/drawable/deferred.frag.spv");
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
    p.addStaticDescriptorSet(0, cameraDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(3, Animation::getDescriptorProvider());
}

void trc::internal::makeDrawableShadowPipeline(RenderPassShadow& renderPass)
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
    vkb::ShaderProgram program(TRC_SHADER_DIR"/drawable/shadow.vert.spv",
                               TRC_SHADER_DIR"/drawable/shadow.frag.spv");

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

void trc::internal::makeInstancedDrawableDeferredPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& cameraDescriptorSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout> {
            cameraDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
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
    vkb::ShaderProgram program(TRC_SHADER_DIR"/drawable/instanced.vert.spv",
                               TRC_SHADER_DIR"/drawable/deferred.frag.spv");

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
    p.addStaticDescriptorSet(0, cameraDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, SceneDescriptor::getProvider());
}

void trc::internal::makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass)
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
    vkb::ShaderProgram program(TRC_SHADER_DIR"/drawable/shadow_instanced.vert.spv",
                               TRC_SHADER_DIR"/drawable/shadow.frag.spv");

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

void trc::internal::makeFinalLightingPipeline(
    RenderPass& renderPass,
    const DescriptorProviderInterface& generalDescriptorSet,
    const DescriptorProviderInterface& gBufferInputSet)
{
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto extent = swapchain.getImageExtent();

    // Layout
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>
        {
            generalDescriptorSet.getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            gBufferInputSet.getDescriptorSetLayout(),
            SceneDescriptor::getProvider().getDescriptorSetLayout(),
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>
        {
            // Camera position
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, sizeof(vec3)),
        }
    );

    // Pipeline
    vkb::ShaderProgram program(TRC_SHADER_DIR"/final_lighting.vert.spv",
                               TRC_SHADER_DIR"/final_lighting.frag.spv");

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
            *vkb::VulkanBase::getDevice(),
            *layout,
            *renderPass, DeferredSubPasses::eLightingPass
        );

    auto& p = makeGraphicsPipeline(Pipelines::eFinalLighting, std::move(layout), std::move(pipeline));
    p.addStaticDescriptorSet(0, generalDescriptorSet);
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, gBufferInputSet);
    p.addStaticDescriptorSet(3, SceneDescriptor::getProvider());
    p.addStaticDescriptorSet(4, ShadowDescriptor::getProvider());
}
