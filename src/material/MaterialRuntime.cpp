#include "trc/material/MaterialRuntime.h"



namespace trc
{

MaterialRuntime::MaterialRuntime(Pipeline::ID pipeline, s_ptr<std::vector<ui32>> pcOffsets)
    :
    pipeline(pipeline),
    pcOffsets(pcOffsets)
{
}

auto MaterialRuntime::getPipeline() const -> Pipeline::ID
{
    return pipeline;
}

} // namespace trc
