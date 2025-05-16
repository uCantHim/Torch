#pragma once

#include <expected>
#include <generator>
#include <optional>
#include <string>
#include <vector>

#include <trc/material/shader/BasicType.h>
#include <trc/material/shader/ShaderModuleBuilder.h>

#include "types.h"

namespace cloth
{
    //namespace code = trc::shader::code;

    namespace resource_references
    {
        /**
         * Cloth code: $texture["/path/to/texture"]
         *
         * Implementation:
         *     <path-string> -> AssetReference -> RuntimeTextureIndex
         */
        struct Texture
        {
        };
    }

    struct Builtin
    {
        enum class ArgType {
            eValue, eTexturePath,
        };

        using ArgValue = std::variant<
            trc::shader::code::Value,     // Identifiers or expressions passed as arguments
            resource_references::Texture  // A texture path string?
            // ...
        >;

        // Fully-qualified ID, i.e., including namespaces
        std::string fullId;

        trc::shader::BasicType type;
        std::optional<std::vector<ArgType>> args{ std::nullopt };
    };

    struct BuiltinDocumentation
    {
        std::string name;
        std::string description;
    };

    struct BuiltinImplementationError
    {
        enum class Code
        {
            eNotFound,
            eNotImplemented,
            eInvalidArguments,
        };

        Code code;
        std::string message;
    };

    /**
     * Implements Cloth built-ins by translating them to shader code, perhaps
     * utilizing capabilities to do so.
     */
    class BuiltinProvider
    {
    public:
        /**
         * @param std::string id The corresponding `Builtin::shaderId` field.
         */
        auto getDefinition(const FullId& id) -> const Builtin*;

        auto getAllDefinitions() const -> std::generator<const Builtin&>;

        static bool validateArgs(const Builtin& builtin,
                                 const std::vector<Builtin::ArgValue>& args);

        auto makeValue(const FullId& builtinId,
                       const std::vector<Builtin::ArgValue>& args,
                       trc::shader::ShaderModuleBuilder& builder)
            -> std::expected<trc::shader::code::Value, BuiltinImplementationError>;
    };
} // namespace cloth
