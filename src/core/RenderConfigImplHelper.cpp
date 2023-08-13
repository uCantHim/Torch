#include "trc/core/RenderConfigImplHelper.h"



namespace trc
{

RenderConfigImplHelper::RenderConfigImplHelper(
    const Instance& instance,
    RenderGraph graph)
    :
    RenderConfig(std::move(graph))
{
    if (pipelineStorage == nullptr) {
        pipelineStorage = PipelineRegistry::makeStorage(instance, *this);
    }
    ++instanceCount;
}

RenderConfigImplHelper::~RenderConfigImplHelper()
{
    /**
     * The pipeline storage is static so that we have only one pipeline
     * instance for all render configs of the same type, but we still
     * have to destroy the pipeline storage before the device is
     * destroyed.
     */
    --instanceCount;
    if (instanceCount == 0) {
        pipelineStorage.reset();
    }
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
