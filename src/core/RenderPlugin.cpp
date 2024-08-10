#include "trc/core/RenderPlugin.h"

#include "trc/core/RenderPipeline.h"



namespace trc
{

PluginBuildContext::PluginBuildContext(
    const Instance& instance,
    RenderPipeline& parentPipeline)
    :
    _instance(instance),
    _parent(parentPipeline)
{
}

auto PluginBuildContext::instance() const -> const Instance&
{
    return _instance;
}

auto PluginBuildContext::device() const -> const Device&
{
    return _instance.getDevice();
}

auto PluginBuildContext::maxViewports() const -> ui32
{
    return _parent.getMaxViewports();
}

auto PluginBuildContext::maxRenderTargetFrames() const -> ui32
{
    return _parent.getRenderTarget().getFrameClock().getFrameCount();
}

auto PluginBuildContext::maxPluginViewportInstances() const -> ui32
{
    return maxViewports() * maxRenderTargetFrames();
}

auto PluginBuildContext::renderTarget() const -> const RenderTarget&
{
    return _parent.getRenderTarget();
}

} // namespace trc
