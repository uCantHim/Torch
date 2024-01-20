#pragma once

#include "trc/core/DescriptorRegistry.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/core/RenderPassRegistry.h"

namespace trc
{
    class ResourceConfig : public RenderPassRegistry
                         , public DescriptorRegistry
    {
    };

    class ResourceStorage
    {
    public:
        ResourceStorage(const ResourceConfig* config, s_ptr<PipelineStorage> pipelines);

        /**
         * @brief Retrieve a pipeline
         *
         * The pipeline is created lazily if it does not exist at the time this
         * function is called.
         *
         * @return Pipeline&
         */
        auto getPipeline(Pipeline::ID id) -> Pipeline&;

        /**
         * @brief Retrieve a descriptor
         *
         * @return `nullptr` if no resource is defined for descriptor ID `id`.
         *         A valid descriptor provider otherwise.
         */
        auto getDescriptor(DescriptorID id) const noexcept
            -> s_ptr<const DescriptorProviderInterface>;

    private:
        s_ptr<PipelineStorage> pipelines;
        DescriptorStorage descriptors;
    };
} // namespace trc
