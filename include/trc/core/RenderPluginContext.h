#pragma once

namespace trc
{
    class Camera;
    class Device;
    class RenderGraph;
    class RenderPipeline;
    class RenderTarget;
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
     */
    class RenderPipelineInfo
    {
    public:
        RenderPipelineInfo(const Device& device, RenderPipeline& pipeline);

        auto device() const -> const Device&;

        auto resourceConfig() -> ResourceConfig&;
        auto renderGraph() -> RenderGraph&;

        auto renderTarget() -> const RenderTarget&;

    private:
        const Device& _device;
        RenderPipeline& _pipeline;
    };

    /**
     * @brief Everything there is to know about a scene.
     */
    class SceneInfo
    {
    public:
        SceneInfo(SceneBase& _scene) : _scene(_scene) {}

        auto scene() -> SceneBase&;

    private:
        SceneBase& _scene;
    };

    /**
     * @brief Everything there is to know about a viewport.
     *
     * Note: Is currently used by ViewportContext and ViewportDrawContext to
     * provide consistent interfaces and reduce boilerplate code.
     */
    class ViewportInfo
    {
    public:
        ViewportInfo(const Viewport& vp,
                     SceneBase& scene,
                     Camera& camera);

        auto viewport() -> const Viewport&;
        auto renderImage() -> const RenderImage&;
        auto renderArea() -> const RenderArea&;

        auto scene() -> SceneBase&;
        auto camera() -> Camera&;

    private:
        const Viewport& _vp;
        SceneBase& _scene;
        Camera& _camera;
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
