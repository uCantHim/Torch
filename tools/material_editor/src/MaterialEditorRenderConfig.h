#pragma once

#include <optional>

#include <trc/ImguiIntegration.h>
#include <trc/core/Instance.h>
#include <trc/core/RenderConfigImplHelper.h>
#include <trc/core/RenderConfiguration.h>
#include <trc/core/RenderStage.h>
#include <trc/core/RenderTarget.h>
#include <trc/text/Font.h>
#include <trc/ui/torch/GuiIntegration.h>

using namespace trc::basic_types;

#include "CameraDescriptor.h"
#include "GraphRenderer.h"

struct MaterialEditorRenderingInfo
{
    const trc::Image& fontImage;

    // A barrier on the render target inserted before rendering to it.
    //
    // The barrier must bring the image into the color attachment optimal
    // layout. The image view can be left blank as it will be set by the
    // renderer.
    std::optional<vk::ImageMemoryBarrier2> renderTargetBarrier;

    // The layout in which the render target's images should be after the
    // material editor's draw commands are executed.
    vk::ImageLayout finalLayout{ vk::ImageLayout::eGeneral };
};

class MaterialEditorRenderPass : public trc::RenderPass
{
public:
    MaterialEditorRenderPass(const trc::RenderTarget& target,
                             const trc::Device& device,
                             const MaterialEditorRenderingInfo& info,
                             trc::RenderConfig& renderConfig);

    void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, trc::FrameRenderState&) override;
    void end(vk::CommandBuffer cmdBuf) override;

    void setViewport(ivec2 offset, uvec2 size);
    void setRenderTarget(const trc::RenderTarget& newTarget);
    auto getRenderer() -> MaterialGraphRenderer&;

private:
    static constexpr vk::ClearColorValue kClearValue{ 0.08f, 0.08f, 0.08f, 1.0f };

    trc::RenderConfig* renderConfig;

    const trc::RenderTarget* renderTarget;
    vk::Rect2D area;
    std::optional<vk::ImageMemoryBarrier2> renderTargetBarrier;
    vk::ImageLayout finalLayout;

    MaterialGraphRenderer renderer;
};

/**
 * @brief Render configuration for the material editor's custom rendering
 *        algorithm
 *
 * Deferred shading is completely overkill for the material editor, so it
 * implements its own simplified rendering algorithm: a classical forward
 * approach.
 */
class MaterialEditorRenderConfig : public trc::RenderConfigImplHelper
{
public:
    static inline auto kMainRenderStage = trc::makeRenderStage();

    static constexpr auto kForwardRenderpass{ "material_editor_forward_renderpass" };
    static constexpr auto kCameraDescriptor{ "material_editor_camera_descriptor" };
    static constexpr auto kTextureDescriptor{ "material_editor_texture_descriptor" };

    MaterialEditorRenderConfig(const trc::RenderTarget& renderTarget,
                               trc::Window& window,
                               const MaterialEditorRenderingInfo& info);

    void update(const trc::Camera& camera, const GraphRenderData& data);

    void setViewport(ivec2 offset, uvec2 size);
    void setRenderTarget(const trc::RenderTarget& newTarget);

    auto getCameraDescriptor() const -> const trc::DescriptorProviderInterface&;

private:
    CameraDescriptor cameraDesc;
    s_ptr<trc::SharedDescriptorSet> textureDesc;
    s_ptr<trc::FontRegistry> fontRegistry;
    std::optional<trc::FontHandle> font;

    MaterialEditorRenderPass renderPass;

    u_ptr<trc::imgui::ImguiRenderPass> imgui;
};
