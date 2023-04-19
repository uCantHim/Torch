#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "BasicType.h"
#include "ShaderCapabilities.h"
#include "ShaderCodeBuilder.h"
#include "trc/Types.h"
#include "trc/util/Pathlet.h"

namespace trc
{
    class ShaderCodeBuilder;

    /**
     * @brief Defines an input/output interface for a single shader module
     *
     * Defines capabilities and resources.
     *
     * A resource is a plain description of a shader input. This may be one of
     *  - shader input attribute
     *  - descriptor set binding
     *  - push constant value
     *
     * Resources are identified by numerical IDs that are generated by the
     * `ShaderCapabilityConfig`.
     *
     * A capability is a possibly complex interface to one or more resources.
     * This may range from a simple access to an input attribute up to
     * complicated computations involving multiple resources and other
     * capabilites.
     *
     * Capabilities are identified by user-specified string identifiers.
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
            ui32 location;
            bool flat{ false };
        };

        struct PushConstant
        {
            PushConstant(BasicType type, ui32 userId)
                : byteSize(type.size()), typeName(type.to_string()), userId(userId) {}
            PushConstant(ui32 size, const std::string& typeName, ui32 userId)
                : byteSize(size), typeName(typeName), userId(userId) {}

            ui32 byteSize;
            std::string typeName;

            ui32 userId;
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
            std::string resourceMacroName;

            std::unordered_set<std::string> extensions;
            std::unordered_set<util::Pathlet> includeFiles;

            /** Maps [name -> value?] */
            std::unordered_map<std::string, std::optional<std::string>> macroDefinitions;
        };

        using ResourceID = ui32;

        auto getCodeBuilder() -> ShaderCodeBuilder&;

        void addGlobalShaderExtension(std::string extensionName);
        void addGlobalShaderInclude(util::Pathlet includePath);
        auto getGlobalShaderExtensions() const -> const std::unordered_set<std::string>&;
        auto getGlobalShaderIncludes() const -> const std::unordered_set<util::Pathlet>&;

        auto addResource(Resource shaderResource) -> ResourceID;
        void addShaderExtension(ResourceID resource, std::string extensionName);
        void addShaderInclude(ResourceID resource, util::Pathlet includePath);
        void addMacro(ResourceID resource, std::string name, std::optional<std::string> value);

        auto accessResource(ResourceID resource) const -> code::Value;
        auto getResource(ResourceID resource) const -> const ResourceData&;

        void linkCapability(Capability capability, ResourceID resource);
        void linkCapability(Capability capability,
                            code::Value value,
                            std::vector<ResourceID> resources);

        /**
         * @return bool True if `capability` is linked to a resource
         */
        bool hasCapability(Capability capability) const;

        auto accessCapability(Capability capability) const -> code::Value;
        auto getCapabilityResources(Capability capability) const -> std::vector<ResourceID>;

    private:
        struct BuiltinConstantInfo
        {
            BasicType type;
            Capability capability;
        };

        s_ptr<ShaderCodeBuilder> codeBuilder{ new ShaderCodeBuilder };

        std::unordered_set<std::string> globalExtensions;
        std::unordered_set<util::Pathlet> globalIncludes;
        std::unordered_set<util::Pathlet> postResourceIncludes;

        std::vector<s_ptr<ResourceData>> resources;
        std::unordered_map<ResourceID, code::Value> resourceAccessors;

        std::unordered_map<Capability, std::unordered_set<ResourceID>> requiredResources;
        std::unordered_map<Capability, code::Value> capabilityAccessors;
    };
} // namespace trc
