#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <trc_util/data/IdPool.h>
#include <trc_util/data/IndexMap.h>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc
{
    class DescriptorProviderInterface;

    /**
     * @brief Strong type to reference a descriptor
     */
    struct DescriptorName
    {
        std::string identifier;
    };

    struct _DescriptorIdTypeTag {};
    using DescriptorID = data::TypesafeID<_DescriptorIdTypeTag, ui32>;

    /**
     * @brief Maps names to descriptors
     *
     * Pipeline layouts store descriptor IDs to reference descriptors that have
     * to be determined dynamically at command recording time.
     *
     * Pipeline layout templates store descriptor names and translate them to
     * descriptor IDs at layout creation.
     *
     * There is no semantical difference between descriptor names and descriptor
     * IDs. The IDs exist because we don't need a hash table to map from IDs to
     * descriptors (but we do when mapping strings to descriptors), which makes
     * the lookup faster.
     */
    class DescriptorRegistry
    {
    public:
        /**
         * @brief Create a descriptor definition
         *
         * @param vk::DescriptorSetLayout layout The descriptor's layout. Must
         *                                       not be `VK_NULL_HANDLE`.
         */
        void defineDescriptor(const DescriptorName& name, vk::DescriptorSetLayout layout);
                              // TODO: Do I want this as an argument?
                              //s_ptr<const DescriptorProviderInterface> setProvider)

        /**
         * @brief Set a descriptor set provider for a descriptor
         *
         * The descriptor provider is used dynamically when pipelines are bound
         * to a command buffer which reference a descriptor statically via a
         * `DescriptorName` handle.
         *
         * @throw Exception if no descriptor with the name `name` is defined at
         *                  the descriptor registry.
         */
        void provideDescriptor(const DescriptorName& name,
                               s_ptr<const DescriptorProviderInterface> provider);

        /**
         * Static usage during pipeline layout creation
         *
         * @throw Exception if no descriptor with the name `name` is defined at
         *                  the descriptor registry.
         */
        auto getDescriptorLayout(const DescriptorName& name) const -> vk::DescriptorSetLayout;

        /**
         * Descriptor IDs are used internally by pipeline layouts to store
         * references to statically used descriptor sets.
         *
         * @throw Exception if no descriptor with the name `name` is defined at
         *                  the descriptor registry.
         */
        auto getDescriptorID(const DescriptorName& name) const -> DescriptorID;

        /**
         * Used internally during command recording/pipeline binding.
         *
         * @throw Exception if no descriptor provider is set for the descriptor
         *                  at `id`.
         */
        auto getDescriptor(DescriptorID id) const -> s_ptr<const DescriptorProviderInterface>;

    private:
        auto getId(const DescriptorName& name) -> DescriptorID;
        auto tryInsertName(const DescriptorName& name) -> DescriptorID;

        data::IdPool<ui32> descriptorIdPool;
        std::unordered_map<std::string, DescriptorID> idPerName;

        data::IndexMap<DescriptorID, vk::DescriptorSetLayout> descriptorSetLayouts;
        data::IndexMap<DescriptorID, s_ptr<const DescriptorProviderInterface>> descriptorProviders;
    };
} // namespace trc
