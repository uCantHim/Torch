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

auto getPipeline(DrawablePipelineTypeFlags flags) -> Pipeline::ID
{
    return getDrawablePipeline(flags);
}

} // namespace trc
