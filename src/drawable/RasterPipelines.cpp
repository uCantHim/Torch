#include "drawable/RasterPipelines.h"

#include "AnimationEngine.h"



namespace trc
{

struct DrawablePushConstants
{
    mat4 model{ 1.0f };
    ui32 materialIndex{ 0u };

    ui32 animationIndex{ NO_ANIMATION };
    uvec2 keyframes{ 0, 0 };
    float keyframeWeight{ 0.0f };
};

auto getPipeline(pipelines::DrawablePipelineTypeFlags flags) -> Pipeline::ID
{
    [[maybe_unused]]
    static bool _init = []{
        pipelines::initDrawablePipelines({ DrawablePushConstants{} });
        return true;
    }();

    return getDrawablePipeline(flags);
}

} // namespace trc
