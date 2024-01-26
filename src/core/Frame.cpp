#include "trc/core/Frame.h"

#include "trc/core/RenderConfiguration.h"



namespace trc
{

auto Frame::addViewport(ViewportConfig& config, SceneBase& scene) -> DrawGroup&
{
    return drawGroups.emplace_back(&config.getResources(), &scene, TaskQueue{});
}

} // namespace trc
