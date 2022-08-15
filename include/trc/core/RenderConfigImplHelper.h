#pragma once

#include "RenderConfiguration.h"
#include "PipelineRegistry.h"

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
        RenderConfigImplHelper(const Instance& instance, RenderLayout layout);
        ~RenderConfigImplHelper();

        auto getPipeline(Pipeline::ID id) -> Pipeline& override;
        auto getPipelineStorage() -> PipelineStorage&;

    private:
        static inline std::atomic<ui32> instanceCount{ 0 };
        static inline u_ptr<PipelineStorage> pipelineStorage{ nullptr };
    };
} // namespace trc
