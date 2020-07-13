#pragma once

#include <array>
#include <vector>

#include <vkb/ShaderProgram.h>

#include "Boilerplate.h"

namespace trc
{
    constexpr vk::PipelineInputAssemblyStateCreateInfo DEFAULT_INPUT_ASSEMBLY(
        vk::PipelineInputAssemblyStateCreateFlags(),
        vk::PrimitiveTopology::eTriangleList,
        false
    );

    constexpr vk::PipelineTessellationStateCreateInfo DEFAULT_TESSELLATION(
        vk::PipelineTessellationStateCreateFlags(),
        0
    );

    constexpr vk::PipelineRasterizationStateCreateInfo DEFAULT_RASTERIZATION(
        vk::PipelineRasterizationStateCreateFlags(),
        false,
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        false, 0.0f, 0.0f, 0.0f, // Depth bias
        1.0f
    );

    constexpr vk::PipelineMultisampleStateCreateInfo DEFAULT_MULTISAMPLING(
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1,
        false, 0.0f,
        nullptr,
        false,
        false
    );

    constexpr vk::PipelineDepthStencilStateCreateInfo DEFAULT_DEPTH_STENCIL(
        vk::PipelineDepthStencilStateCreateFlags(),
        true,
        true,
        vk::CompareOp::eLess,
        false,
        // Stencil
        false,
        vk::StencilOpState(), vk::StencilOpState(),
        // Depth bounds
        0.0f, 1.0f
    );

    constexpr vk::PipelineColorBlendAttachmentState DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED(
        VK_FALSE,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );

    class GraphicsPipelineBuilder
    {
    private:
        GraphicsPipelineBuilder() = default;

    public:
        using Self = GraphicsPipelineBuilder;

        static auto create() -> GraphicsPipelineBuilder;

        auto setProgram(const vkb::ShaderProgram& program) -> Self;

        auto addVertexInputBinding(vk::VertexInputBindingDescription inputBinding,
                                   std::vector<vk::VertexInputAttributeDescription> attributes) -> Self;
        auto setVertexInput(const std::vector<vk::VertexInputBindingDescription>& inputBindings,
                            const std::vector<vk::VertexInputAttributeDescription>& attributes) -> Self;

        auto setInputAssembly(vk::PipelineInputAssemblyStateCreateInfo inputAssembly) -> Self;
        auto setPrimitiveTopology(vk::PrimitiveTopology topo) -> Self;

        auto setTessellation(vk::PipelineTessellationStateCreateInfo tessellation) -> Self;

        auto addViewport(vk::Viewport viewport) -> Self;
        auto addScissorRect(vk::Rect2D scissorRect) -> Self;

        auto setRasterization(vk::PipelineRasterizationStateCreateInfo rasterization) -> Self;
        auto setPolygonMode(vk::PolygonMode polyMode) -> Self;
        auto setCullMode(vk::CullModeFlags cullMode) -> Self;
        auto setFrontFace(vk::FrontFace frontFace) -> Self;

        auto setMultisampling(vk::PipelineMultisampleStateCreateInfo multisampling) -> Self;
        auto setSampleCount(vk::SampleCountFlagBits sampleCount) -> Self;

        auto setDepthStencilTests(vk::PipelineDepthStencilStateCreateInfo depthStencil) -> Self;
        auto enableDepthTest() -> Self;
        auto disableDepthTest() -> Self;

        auto addColorBlendAttachment(vk::PipelineColorBlendAttachmentState blendAttachment) -> Self;
        /**
         * Call this after all color blend attachments have been specified via
         * addColorBlendAttachment().
         */
        auto setColorBlending(vk::PipelineColorBlendStateCreateFlags flags,
                              vk::Bool32 logicOpEnable, vk::LogicOp logicalOperation,
                              std::array<float, 4> blendConstants) -> Self;

        auto addDynamicState(vk::DynamicState dynamicState) -> Self;

        /**
         * @build Finish the build process and create a pipeline
         */
        auto build(vk::Device device,
                   vk::PipelineLayout layout,
                   vk::RenderPass renderPass,
                   ui32 subPass)
            -> vk::UniquePipeline;

    private:
        const vkb::ShaderProgram* program;

        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;
        vk::PipelineVertexInputStateCreateInfo vertexInput;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ DEFAULT_INPUT_ASSEMBLY };

        vk::PipelineTessellationStateCreateInfo tessellation{ DEFAULT_TESSELLATION };

        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D> scissorRects;
        vk::PipelineViewportStateCreateInfo viewport;

        vk::PipelineRasterizationStateCreateInfo rasterization{ DEFAULT_RASTERIZATION };
        vk::PipelineMultisampleStateCreateInfo multisampling{ DEFAULT_MULTISAMPLING };
        vk::PipelineDepthStencilStateCreateInfo depthStencil{ DEFAULT_DEPTH_STENCIL };

        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        vk::PipelineColorBlendStateCreateInfo colorBlending;

        std::vector<vk::DynamicState> dynamicStates;
        vk::PipelineDynamicStateCreateInfo dynamicState;
    };
} // namespace trc
