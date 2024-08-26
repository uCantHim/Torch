#pragma once

#include "trc/core/RenderTarget.h"

namespace trc
{
    class Camera;
    class Device;
    class RenderGraph;
    class RenderPipeline;
    class ResourceConfig;
    class SceneBase;
    struct RenderArea;
    struct RenderImage;
    struct Viewport;
} // namespace trc

namespace trc::impl
{
    /**
     * @brief Everything there is to know about a render pipeline's state.
     *
     * This is still a *frame specific* structure and holds information for a
     * specific instance of a pipeline for a specific frame.
     */
    class RenderPipelineInfo
    {
    public:
        RenderPipelineInfo(const Device& device,
                           const RenderImage& renderTargetImage,
                           RenderPipeline& pipeline);

        auto device() const -> const Device&;

        auto resourceConfig() -> ResourceConfig&;
        auto renderGraph() -> RenderGraph&;

        auto renderTargetImage() -> const RenderImage&;

    private:
        const Device& _device;
        const RenderImage _image;
        RenderPipeline& _pipeline;  // For global information that is not frame
                                    // specific.
    };

    /**
     * @brief Everything there is to know about a scene.
     */
    class SceneInfo
    {
    public:
        SceneInfo(const s_ptr<SceneBase>& _scene);

        auto scene() -> SceneBase&;

    private:
        s_ptr<SceneBase> _scene;
    };

    /**
     * @brief Everything there is to know about a viewport.
     *
     * Note: Is currently used by ViewportContext and ViewportDrawContext to
     * provide consistent interfaces and reduce boilerplate.
     */
    class ViewportInfo
    {
    public:
        ViewportInfo(const Viewport& vp,
                     const s_ptr<Camera>& camera,
                     const s_ptr<SceneBase>& scene);

        auto viewport() const -> const Viewport&;
        auto renderImage() const -> const RenderImage&;
        auto renderArea() const -> const RenderArea&;

        auto scene() -> SceneBase&;
        auto camera() -> Camera&;

    private:
        Viewport _vp;
        s_ptr<Camera> _camera;
        s_ptr<SceneBase> _scene;
    };
} // namespace trc::impl

namespace trc
{
    /**
     * @brief Used to provide context information to render plugins when dealing
     *        with viewports.
     */
    struct RenderPipelineContext : public impl::RenderPipelineInfo
    {
    public:
        explicit RenderPipelineContext(const impl::RenderPipelineInfo& info)
            : impl::RenderPipelineInfo(info) {}
    };

    /**
     * @brief Used to provide context information to render plugins when dealing
     *        with viewports.
     */
    struct SceneContext : public impl::RenderPipelineInfo
                        , public impl::SceneInfo
    {
    public:
        SceneContext(const impl::RenderPipelineInfo& info,
                     const impl::SceneInfo& vpInfo)
            : impl::RenderPipelineInfo(info), impl::SceneInfo(vpInfo) {}
    };

    /**
     * @brief Used to provide context information to render plugins when dealing
     *        with viewports.
     */
    struct ViewportContext : public impl::RenderPipelineInfo
                           , public impl::ViewportInfo
    {
    public:
        ViewportContext(const impl::RenderPipelineInfo& info,
                        const impl::ViewportInfo& vpInfo)
            : impl::RenderPipelineInfo(info), impl::ViewportInfo(vpInfo) {}
    };
} // namespace trc
