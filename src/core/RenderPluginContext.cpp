#include "trc/core/RenderPluginContext.h"

#include "trc/core/RenderPipeline.h"



namespace trc
{

impl::RenderPipelineInfo::RenderPipelineInfo(
    const Device& device,
    const RenderImage& renderTargetImage,
    RenderPipeline& pipeline)
    :
    _device(device),
    _image(renderTargetImage),
    _pipeline(pipeline)
{
}

auto impl::RenderPipelineInfo::device() const -> const Device&
{
    return _device;
}

auto impl::RenderPipelineInfo::resourceConfig() -> ResourceConfig&
{
    return _pipeline.getResourceConfig();
}

auto impl::RenderPipelineInfo::renderGraph() -> RenderGraph&
{
    return _pipeline.getRenderGraph();
}

auto impl::RenderPipelineInfo::renderTargetImage() -> const RenderImage&
{
    return _image;
}



impl::SceneInfo::SceneInfo(const s_ptr<SceneBase>& scene)
    : _scene(scene)
{
    assert_arg(scene != nullptr);
}

auto impl::SceneInfo::scene() -> SceneBase&
{
    return *_scene;
}



impl::ViewportInfo::ViewportInfo(
    const Viewport& vp,
    vec4 clearColor,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene)
    :
    _vp(vp),
    _clearColor(clearColor),
    _camera(camera),
    _scene(scene)
{
    assert_arg(camera != nullptr);
    assert_arg(scene != nullptr);
}

auto impl::ViewportInfo::viewport() const -> const Viewport&
{
    return _vp;
}

auto impl::ViewportInfo::renderImage() const -> const RenderImage&
{
    return _vp.target;
}

auto impl::ViewportInfo::renderArea() const -> const RenderArea&
{
    return _vp.area;
}

auto impl::ViewportInfo::clearColor() const -> vec4
{
    return _clearColor;
}

auto impl::ViewportInfo::scene() -> SceneBase&
{
    return *_scene;
}

auto impl::ViewportInfo::camera() -> Camera&
{
    return *_camera;
}

} // namespace trc
