#pragma once

#include "trc/core/Instance.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/core/RenderConfiguration.h"

namespace trc
{
    /**
     * @brief An implementation helper for custom RenderConfigs
     *
     * Implements RenderConfig::getPipeline via a global PipelineStorage
     * object.
     */
    class RenderConfigImplHelper : public RenderConfig
    {
    public:
        RenderConfigImplHelper(const Instance& instance, RenderGraph graph);

        auto getPipeline(Pipeline::ID id) -> Pipeline& override;
        auto getPipelineStorage() -> PipelineStorage&;

    private:
        u_ptr<PipelineStorage> pipelineStorage;
    };
} // namespace trc
