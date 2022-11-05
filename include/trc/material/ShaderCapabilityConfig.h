#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "BasicType.h"
#include "ShaderCapabilities.h"
#include "trc/Types.h"

namespace trc
{
    enum class Builtin
    {
        eVertexPosition,
        eVertexNormal,
        eVertexUV,

        eTime,
        eTimeDelta,

        eNumBuiltinConstants,
    };

    /**
     * Settings for shader generation
     */
    class ShaderCapabilityConfig
    {
    public:
        struct DescriptorBinding
        {
            std::string setName;

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

        using ResourceID = ui32;

        auto addResource(Resource shaderResource) -> ResourceID;
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
        auto getResource(Capability capability) const -> std::optional<const Resource*>;

        /**
         * Configure how a builtin constant can be accessed in the shader
         *
         * @param std::string accessorCode Is relative to the backing
         *        resource's name.
         */
        void setConstantAccessor(Builtin constant, std::string accessorCode);
        auto getConstantAccessor(Builtin constant) const -> std::optional<std::string>;

        static auto getConstantCapability(Builtin constant) -> Capability;
        static auto getConstantType(Builtin constant) -> BasicType;

    private:
        struct BuiltinConstantInfo
        {
            BasicType type;
            Capability capability;
        };

        static const std::array<
            BuiltinConstantInfo,
            static_cast<size_t>(Builtin::eNumBuiltinConstants)
        > builtinConstants;

        std::vector<Resource> resources;
        std::unordered_map<Capability, ResourceID> resourceFromCapability;
        std::unordered_map<Builtin, std::string> accessorFromBuiltinConstant;
    };
} // namespace trc
