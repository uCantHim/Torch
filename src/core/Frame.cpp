#include "trc/core/Frame.h"



namespace trc
{

Frame::Frame(const Device* device)
    : device(device)
{
}

auto Frame::addViewport(RenderConfig& config, SceneBase& scene) -> DrawGroup&
{
    return drawGroups.emplace_back(&config, &scene, TaskQueue{});
}

} // namespace trc
