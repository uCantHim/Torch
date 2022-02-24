#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include <trc_util/data/ObjectId.h>
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

    struct _DescriptorDefinition
    {
        DescriptorName name;

        vk::DescriptorSetLayout layout;
        std::function<vk::DescriptorSetLayout()> layoutGetter;
    };

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
        struct DescriptorDefinition
        {
            vk::DescriptorSetLayout layout;
        };

        /**
         * @brief Register a descriptor
         */
        auto addDescriptor(DescriptorName name, const DescriptorProviderInterface& provider)
            -> DescriptorID;

        /**
         * Static usage during pipeline layout creation
         */
        auto getDescriptorLayout(const DescriptorName& name) const -> vk::DescriptorSetLayout;

        /**
         * Static usage at pipeline layout creation
         */
        static auto getDescriptorID(const DescriptorName& name) -> DescriptorID;

        /**
         * Dynamic usage during command recording
         */
        auto getDescriptor(DescriptorID id) const -> const DescriptorProviderInterface&;

    private:
        static auto tryInsertName(const DescriptorName& name) -> DescriptorID;

        static inline data::IdPool descriptorIdPool;
        static inline std::unordered_map<std::string, DescriptorID> idPerName;

        data::IndexMap<DescriptorID, const DescriptorProviderInterface*> descriptorProviders;
    };
} // namespace trc
