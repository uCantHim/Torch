#include "drawable/RasterPipelines.h"

#include <vector>

#include "core/PipelineBuilder.h"
#include "core/PipelineLayoutBuilder.h"
#include "PipelineDefinitions.h"
#include "TorchRenderConfig.h"
#include "AnimationEngine.h"



namespace trc
{

auto makeVertexBindingDescription(PipelineVertexTypeFlagBits type)
    -> vk::VertexInputBindingDescription
{
    const size_t stride = type == PipelineVertexTypeFlagBits::eSkeletal
        ? sizeof(MeshVertex) + sizeof(SkeletalVertex)
        : sizeof(MeshVertex);

    return vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex);
}

auto makeVertexAttributeDescriptions(PipelineVertexTypeFlagBits type)
    -> std::vector<vk::VertexInputAttributeDescription>
{
    if (type == PipelineVertexTypeFlagBits::eSkeletal)
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



auto makeDrawableOpaquePipeline(PipelineFlags flags) -> Pipeline::ID;
auto makeDrawableTransparentPipeline(PipelineFlags flags) -> Pipeline::ID;
auto makeDrawableShadowPipeline(PipelineFlags flags) -> Pipeline::ID;

auto getPipeline(PipelineFlags flags) -> Pipeline::ID
{
    static std::array<Pipeline::ID, PipelineFlags::size()> pipelines;

    const size_t index = flags.toIndex();

    // Animated pipeline must have skeletal vertices
    if (flags & PipelineAnimationTypeFlagBits::eAnimated) {
        flags |= PipelineVertexTypeFlagBits::eSkeletal;
    }

    // Shadow
    if (flags & PipelineShadingTypeFlagBits::eShadow)
    {
        if (pipelines[index] == Pipeline::ID::NONE) {
            pipelines[index] = makeDrawableShadowPipeline(flags);
        }
        return pipelines[index];
    }

    // Opaque or transparent
    if (flags & PipelineShadingTypeFlagBits::eTransparent)
    {
        if (pipelines[index] == Pipeline::ID::NONE) {
            pipelines[index] = makeDrawableTransparentPipeline(flags);
        }
        return pipelines[index];
    }
    else
    {
        if (pipelines[index] == Pipeline::ID::NONE) {
            pipelines[index] = makeDrawableOpaquePipeline(flags);
        }
        return pipelines[index];
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

auto makeDrawableOpaquePipeline(PipelineFlags flags) -> Pipeline::ID
{
    const bool isAnimated = flags & PipelineAnimationTypeFlagBits::eAnimated;

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::G_BUFFER_DESCRIPTOR }, true)
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
            makeVertexBindingDescription(flags.get<PipelineVertexTypeFlagBits>()),
            makeVertexAttributeDescriptions(flags.get<PipelineVertexTypeFlagBits>())
        )
        .disableBlendAttachments(3)
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::OPAQUE_G_BUFFER_PASS }
        );
}

auto makeDrawableTransparentPipeline(PipelineFlags flags) -> Pipeline::ID
{
    const bool isAnimated = flags & PipelineAnimationTypeFlagBits::eAnimated;

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ TorchRenderConfig::G_BUFFER_DESCRIPTOR }, true)
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
            makeVertexBindingDescription(flags.get<PipelineVertexTypeFlagBits>()),
            makeVertexAttributeDescriptions(flags.get<PipelineVertexTypeFlagBits>())
        )
        .setCullMode(vk::CullModeFlagBits::eNone) // Don't cull back faces because they're visible
        .disableDepthWrite()
        .registerPipeline<TorchRenderConfig>(
            layout, RenderPassName{ TorchRenderConfig::TRANSPARENT_G_BUFFER_PASS }
        );
}

auto makeDrawableShadowPipeline(PipelineFlags flags) -> Pipeline::ID
{
    const bool isAnimated = flags & PipelineAnimationTypeFlagBits::eAnimated;

    auto layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ TorchRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addDescriptorIf(isAnimated, DescriptorName{ TorchRenderConfig::ASSET_DESCRIPTOR }, true)
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
            makeVertexBindingDescription(flags.get<PipelineVertexTypeFlagBits>()),
            makeVertexAttributeDescriptions(flags.get<PipelineVertexTypeFlagBits>())
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
