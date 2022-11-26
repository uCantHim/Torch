#pragma once

#include "ShaderCapabilityConfig.h"
#include "ShaderCodeBuilder.h"
#include "ShaderResourceInterface.h"

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
        explicit ShaderModuleBuilder(ShaderCapabilityConfig conf);

        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        auto makeCall(std::vector<Value> args) -> Value;

        auto makeCapabilityAccess(Capability capability) -> Value;
        auto makeTextureSample(TextureReference tex, Value uvs) -> Value;

        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        void makeCallStatement(std::vector<code::Value> args);

        auto getCapabilityConfig() const -> const ShaderCapabilityConfig&;
        auto compileResourceDecls() const -> ShaderResources;

        auto getPrimaryBlock() const -> Block;

    private:
        template<std::derived_from<ShaderFunction> T>
            requires std::is_default_constructible_v<T>
        auto createFunctionDef() -> Function;

        ShaderCapabilityConfig config;
        ShaderResourceInterface resources;
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
