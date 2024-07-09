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
         * @throw std::out_of_range if no pipeline with this ID exists.
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

        /**
         * @brief Create a derived resource storage.
         *
         * Create a resource storage by deriving it from another. The derived
         * storage inherits references to all of the parent's resources, but can
         * define its own resources that will overwrite those of the parent.
         *
         * Example:
         * ```
         *
         * auto a = parent->getDescriptor(myDescId);
         *
         * auto derived = ResourceStorage::derive(parent);
         * assert(a == derived->getDescriptor(myDescId));
         *
         * derived->provideDescriptor(myDesc, newDescriptor);
         * assert(a != derived->getDescriptor(myDescId));
         * ```
         *
         * @param s_ptr<ResourceStorage> parent The resource storage from which
         *                                      to derive a new one. Must not
         *                                      be `nullptr`.
         */
        static auto derive(s_ptr<ResourceStorage> parent) -> u_ptr<ResourceStorage>;

        void setParentStorage(s_ptr<ResourceStorage> newParent);

    private:
        /**
         * Construct a derived resource storage
         */
        explicit ResourceStorage(s_ptr<ResourceStorage> parent);

        s_ptr<ResourceStorage> parent{ nullptr };

        s_ptr<ResourceConfig> resourceConfig;

        s_ptr<PipelineStorage> pipelines;
        DescriptorStorage descriptors;
    };
} // namespace trc
