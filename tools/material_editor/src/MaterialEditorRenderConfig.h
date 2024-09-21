#pragma once

#include <optional>

#include <trc/ImguiIntegration.h>
#include <trc/core/Instance.h>
#include <trc/core/RenderPlugin.h>
#include <trc/core/RenderStage.h>
#include <trc/core/RenderTarget.h>
#include <trc/text/Font.h>
#include <trc/ui/torch/GuiIntegration.h>

using namespace trc::basic_types;

#include "CameraDescriptor.h"
#include "GraphRenderer.h"

class MaterialEditorRenderPass
{
public:
    void begin(vk::CommandBuffer cmdBuf, trc::ViewportDrawContext& ctx);
    void end(vk::CommandBuffer cmdBuf, trc::ViewportDrawContext& ctx);

private:
    static constexpr vk::ClearColorValue kClearValue{ 0.08f, 0.08f, 0.08f, 1.0f };
};

/**
 * @brief Render configuration for the material editor's custom rendering
 *        algorithm
 *
 * Deferred shading is completely overkill for the material editor, so it
 * implements its own simplified rendering algorithm: a classical forward
 * approach.
 */
class MaterialEditorRenderPlugin : public trc::RenderPlugin
{
public:
    static inline auto kMainRenderStage = trc::makeRenderStage();

    static constexpr auto kForwardRenderpass{ "material_editor_forward_renderpass" };
    static constexpr auto kCameraDescriptor{ "material_editor_camera_descriptor" };
    static constexpr auto kTextureDescriptor{ "material_editor_texture_descriptor" };

    MaterialEditorRenderPlugin(const trc::Device& device,
                               const trc::RenderTarget& renderTarget);

    void update(const trc::Camera& camera, const GraphRenderData& data);

    void defineRenderStages(trc::RenderGraph& graph) override;
    void defineResources(trc::ResourceConfig& config) override;
    auto createViewportResources(trc::ViewportContext& ctx)
        -> u_ptr<trc::ViewportResources> override;

private:
    class ViewportConfig : public trc::ViewportResources
    {
    public:
        ViewportConfig(MaterialEditorRenderPlugin& parent, trc::ViewportContext& ctx);

        void registerResources(trc::ResourceStorage& resources) override;
        void hostUpdate(trc::ViewportContext& ctx) override;
        void createTasks(trc::ViewportDrawTaskQueue& queue, trc::ViewportContext& ctx) override;

    private:
        MaterialEditorRenderPlugin& parent;
    };

    const vk::Format renderTargetFormat;

    s_ptr<CameraDescriptor> cameraDesc;
    s_ptr<trc::SharedDescriptorSet> textureDesc;
    s_ptr<trc::FontRegistry> fontRegistry;
    std::optional<trc::FontHandle> font;

    MaterialEditorRenderPass renderPass;
    MaterialGraphRenderer renderer;
};
