#pragma once

#include <expected>
#include <generator>
#include <optional>
#include <string>
#include <vector>

#include <trc/assets/AssetPath.h>
#include <trc/material/shader/BasicType.h>
#include <trc/material/shader/ShaderModuleBuilder.h>

#include "full_id.h"

namespace cloth
{
    /**
     * @brief Describes a Cloth builtin.
     *
     * Builtins are an additional semantic layer between the parser and
     * the code generator. They describe Cloth's built-in high-level
     * functionality and the corresponding translation to shader code.
     *
     * Builtins can be conceived of as functions of zero or more arguments that
     * yield exactly one value.
     */
    struct Builtin
    {
        enum class ArgType
        {
            eValue,

            /**
             * Cloth code: $texture["/path/to/texture"]
             *
             * Implementation:
             *     <path-string> -> AssetReference -> RuntimeTextureIndex
             */
            eResourcePath,
        };

        using ArgValue = std::variant<
            trc::shader::code::Value,  // Identifiers or expressions passed as arguments
            trc::AssetPath             // A texture path string?
        >;

        // Fully-qualified ID, i.e., and identifier including namespaces
        std::string fullId;

        // The result type.
        trc::shader::BasicType type;

        // The number of parameters the builtin takes and their respective types.
        std::vector<ArgType> args{};
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
     * Defines all of Cloth's built-ins.
     *
     * Implements Cloth built-ins by translating them to shader code, perhaps
     * utilizing capabilities to do so.
     */
    class BuiltinProvider
    {
    public:
        using BuiltinValueFactory = std::function<
            trc::shader::code::Value(
                const std::vector<Builtin::ArgValue>&,
                trc::shader::ShaderModuleBuilder&
            )
        >;

        explicit BuiltinProvider(std::vector<std::pair<Builtin, BuiltinValueFactory>> builtinDefinitions);

        /**
         * @brief Get the corresponding builtin to a variable identifier.
         *
         * @return `nullptr` if no builtin with the specified name is defined.
         */
        auto getDefinition(const FullId& id) -> const Builtin*;

        auto getAllDefinitions() const -> std::generator<const Builtin&>;

        auto makeValue(const FullId& builtinId,
                       const std::vector<Builtin::ArgValue>& args,
                       trc::shader::ShaderModuleBuilder& builder)
            -> std::expected<trc::shader::code::Value, BuiltinImplementationError>;

    private:
        static bool validateArgs(const Builtin& builtin,
                                 const std::vector<Builtin::ArgValue>& args);

        std::unordered_map<std::string, Builtin> builtinDefinitions;
        std::unordered_map<std::string, BuiltinValueFactory> builtinFactories;
    };

    auto makeVertexBuiltinProvider() -> BuiltinProvider;
    auto makeFragmentBuiltinProvider() -> BuiltinProvider;
} // namespace cloth
