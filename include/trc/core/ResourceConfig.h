#pragma once

#include "trc/Types.h"
#include "trc/core/DescriptorRegistry.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/core/RenderPassRegistry.h"

namespace trc
{
    /**
     * @brief Defines resource descriptions
     */
    class ResourceConfig : public RenderPassRegistry
                         , public DescriptorRegistry
    {
    };

    /**
     * @brief Stores resources
     */
    class ResourceStorage
    {
    public:
        ResourceStorage(s_ptr<ResourceConfig> config, s_ptr<PipelineStorage> pipelines);

        ResourceStorage(const ResourceStorage&) = delete;
        ResourceStorage& operator=(const ResourceStorage&) = delete;

        ResourceStorage(ResourceStorage&&) noexcept = default;
        ResourceStorage& operator=(ResourceStorage&&) noexcept = default;
        ~ResourceStorage() noexcept = default;

        auto getResourceConfig() -> ResourceConfig&;
        auto getResourceConfig() const -> const ResourceConfig&;

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

        /**
         * @brief Provide a resource for a declared descriptor
         *
         * The descriptor provider is used dynamically when pipelines are bound
         * to a command buffer which reference a descriptor statically via a
         * `DescriptorName` handle.
         *
         * @param s_ptr<const DescriptorProviderInterface> provider Must not
         *        be `nullptr`.
         *
         * @throw Exception if no descriptor with name `descName` is defined at
         *                  the parent descriptor registry.
         */
        void provideDescriptor(const DescriptorName& descName,
                               s_ptr<const DescriptorProviderInterface> provider);

    private:
        s_ptr<ResourceConfig> resourceConfig;

        s_ptr<PipelineStorage> pipelines;
        DescriptorStorage descriptors;
    };
} // namespace trc
