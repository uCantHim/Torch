#include "trc/core/Frame.h"

#include "trc/core/RenderConfiguration.h"



namespace trc
{

auto Frame::addViewport(ViewportConfig& config, SceneBase& scene) -> DrawGroup&
{
    auto& group = *drawGroups.emplace_back(
        std::make_unique<DrawGroup>(&config.getResources(), &scene, TaskQueue{})
    );
    config.createTasks(scene, group.taskQueue);

    return group;
}

} // namespace trc
