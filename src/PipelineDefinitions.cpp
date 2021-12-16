#include "PipelineDefinitions.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "core/PipelineRegistry.h"
#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "Vertex.h"
#include "AssetRegistry.h"
#include "DeferredRenderConfig.h"



namespace trc
{

auto makeFinalLightingPipeline() -> PipelineTemplate;

PIPELINE_GETTER_FUNC(getFinalLightingPipeline, makeFinalLightingPipeline, DeferredRenderConfig);





auto makeFinalLightingPipeline() -> PipelineTemplate
{
    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .registerLayout<DeferredRenderConfig>();

    return buildGraphicsPipeline()
        .setProgram(vkb::readFile(SHADER_DIR / "final_lighting.vert.spv"),
                    vkb::readFile(SHADER_DIR / "final_lighting.frag.spv"))
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
        .build(layout, RenderPassName{ DeferredRenderConfig::FINAL_LIGHTING_PASS });
}

} // namespace trc
