#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "BasicType.h"
#include "ShaderCapabilities.h"
#include "trc/Types.h"
#include "trc/util/Pathlet.h"

namespace trc
{
    /**
     * Settings for shader generation
     */
    class ShaderCapabilityConfig
    {
    public:
        struct DescriptorBinding
        {
            std::string setName;
            ui32 bindingIndex;

            std::string descriptorType;
            std::string descriptorName;
            bool isArray;
            ui32 arrayCount{ 0 };

            std::optional<std::string> layoutQualifier;
            std::optional<std::string> descriptorContent;
        };

        struct ShaderInput
        {
            BasicType type;
            bool flat{ false };
        };

        struct PushConstant
        {
            std::string contents;
        };

        using Resource = std::variant<
            DescriptorBinding,
            ShaderInput,
            PushConstant
        >;

        /** A resource and its additional requirements */
        struct ResourceData
        {
            Resource resourceType;

            std::unordered_set<std::string> extensions;
            std::unordered_set<util::Pathlet> includeFiles;
        };

        using ResourceID = ui32;

        auto addResource(Resource shaderResource) -> ResourceID;
        void addShaderExtension(ResourceID resource, std::string extensionName);
        void addShaderInclude(ResourceID resouce, util::Pathlet includePath);

        void linkCapability(Capability capability, ResourceID resource);

        /**
         * @return bool True if `capability` is linked to a resource
         */
        bool hasCapability(Capability capability) const;

        /**
         * Don't insert new resources while holding a pointer to a resource!
         *
         * @return std::optional<const Resource*> None if `capability` is
         *         not linked to a resource, otherwise the linked resource.
         */
        auto getResource(Capability capability) const -> std::optional<const ResourceData*>;

        void setCapabilityAccessor(Capability capability, std::string accessorCode);
        auto getCapabilityAccessor(Capability capability) const -> std::optional<std::string>;

    private:
        struct BuiltinConstantInfo
        {
            BasicType type;
            Capability capability;
        };

        std::vector<ResourceData> resources;
        std::unordered_map<Capability, ResourceID> resourceFromCapability;
        std::unordered_map<Capability, std::string> capabilityAccessors;
    };
} // namespace trc
