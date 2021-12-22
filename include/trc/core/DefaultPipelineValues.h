#pragma once

#include "../Types.h"

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
#ifdef TRC_FLIP_Y_PROJECTION
        vk::FrontFace::eCounterClockwise,
#else
        vk::FrontFace::eClockwise,
#endif
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
} // namespace trc
