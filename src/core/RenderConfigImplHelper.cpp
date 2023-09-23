#include "trc/core/RenderConfigImplHelper.h"



namespace trc
{

RenderConfigImplHelper::RenderConfigImplHelper(
    const Instance& instance,
    RenderGraph graph)
    :
    RenderConfig(std::move(graph)),
    pipelineStorage(PipelineRegistry::makeStorage(instance, *this))
{
}

auto RenderConfigImplHelper::getPipeline(Pipeline::ID id) -> Pipeline&
{
    return pipelineStorage->get(id);
}

auto RenderConfigImplHelper::getPipelineStorage() -> PipelineStorage&
{
    return *pipelineStorage;
}

} // namespace trc
