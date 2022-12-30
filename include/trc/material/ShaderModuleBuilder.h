#pragma once

#include <concepts>
#include <string>
#include <unordered_map>
#include <vector>

#include <spirv/FileIncluder.h>

#include "ShaderCapabilityConfig.h"
#include "ShaderCodeBuilder.h"
#include "ShaderResourceInterface.h"
#include "trc/util/Pathlet.h"

namespace trc
{
    class ShaderModuleBuilder;

    /**
     * A self-contained, typed mechanism to create shader functions. A
     * subclass can implement one GLSL function. Add this function to the
     * code builder with `ShaderModuleBuilder::makeCall`.
     */
    class ShaderFunction
    {
    public:
        ShaderFunction(const std::string& name, FunctionType signature);
        virtual ~ShaderFunction() = default;

        virtual void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) = 0;

        auto getName() const -> const std::string&;
        auto getType() const -> const FunctionType&;

    private:
        const std::string name;
        const FunctionType signature;
    };

    /**
     * An extension of ShaderCodeBuilder with additional functionality to
     * create shader resources.
     */
    class ShaderModuleBuilder : public ShaderCodeBuilder
    {
    public:
        explicit ShaderModuleBuilder(const ShaderCapabilityConfig& conf);

        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        auto makeCall(std::vector<Value> args) -> Value;

        auto makeCapabilityAccess(Capability capability) -> Value;
        auto makeTextureSample(TextureReference tex, Value uvs) -> Value;

        /**
         * @brief Create a function call statement
         *
         * Since the generated instruction is not an expression, it will
         * never be optimized away if the result is not used. Use this to
         * call functions that are purely used for side effects, e.g. image
         * stores.
         */
        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        void makeCallStatement(std::vector<code::Value> args);

        /**
         * @brief Include external code into the module
         *
         * The file contents are included after the resource declarations.
         * Multiple included files are included in the order in which
         * `includeCode` was called.
         *
         * The include path is resolved at module compile time.
         *
         * Allows the specification of additional replacement variables in
         * the included file and corresponding shader capabilities. The
         * variables will be replaced by the accessor to the respective
         * capability.
         *
         * @param util::Pathlet path The path to the included file
         * @param map<string, Capability> Maps replacement-variable names
         *                                to capabilities.
         */
        void includeCode(util::Pathlet path,
                         std::unordered_map<std::string, Capability> varReplacements);

        struct Settings
        {
            bool earlyFragmentTests{ false };
        };

        /**
         * Generate an instruction in the shader that enables early
         * fragment tests.
         */
        void enableEarlyFragmentTest();

        auto getCapabilityConfig() const -> const ShaderCapabilityConfig&;
        auto getSettings() const -> const Settings&;
        auto compileResourceDecls() const -> ShaderResources;
        auto compileIncludedCode(shaderc::CompileOptions::IncluderInterface& includer)
            -> std::string;

        auto getPrimaryBlock() const -> Block;

    private:
        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        auto createFunctionDef() -> Function;

        const ShaderCapabilityConfig config;
        Settings shaderSettings;
        ShaderResourceInterface resources;

        // Keep the includes in insertion order
        std::vector<
            std::pair<util::Pathlet, std::unordered_map<std::string, Value>>
        > includedFiles;
    };



    template<std::derived_from<ShaderFunction> T>
        requires std::is_default_constructible_v<T>
    auto ShaderModuleBuilder::createFunctionDef() -> Function
    {
        if (auto func = getFunction(T{}.getName())) {
            return *func;
        }

        T funcBuilder;
        auto func = makeFunction(funcBuilder.getName(), funcBuilder.getType());
        startBlock(func);
        funcBuilder.build(*this, func->getArgs());
        endBlock();

        return func;
    }

    template<std::derived_from<ShaderFunction> T>
        requires std::is_default_constructible_v<T>
    auto ShaderModuleBuilder::makeCall(std::vector<Value> args) -> Value
    {
        return ShaderCodeBuilder::makeCall(createFunctionDef<T>(), args);
    }

    template<std::derived_from<ShaderFunction> T>
        requires std::is_default_constructible_v<T>
    void ShaderModuleBuilder::makeCallStatement(std::vector<code::Value> args)
    {
        Function func = createFunctionDef<T>();
        makeStatement(code::FunctionCall{ func, std::move(args) });
    }
} // namespace trc
