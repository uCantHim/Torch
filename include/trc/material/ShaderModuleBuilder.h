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

        auto getCapabilityConfig() const -> const ShaderCapabilityConfig&;
        auto compileResourceDecls() const -> ShaderResources;

    private:
        ShaderCapabilityConfig config;
        ShaderResourceInterface resources;
    };



    template<std::derived_from<ShaderFunction> T>
        requires std::is_default_constructible_v<T>
    auto ShaderModuleBuilder::makeCall(std::vector<Value> args) -> Value
    {
        if (auto func = getFunction(T{}.getName())) {
            return ShaderCodeBuilder::makeCall(*func, std::move(args));
        }

        T funcBuilder;
        auto func = makeFunction(funcBuilder.getName(), funcBuilder.getType());
        startBlock(func);
        funcBuilder.build(*this, func->getArgs());
        endBlock();

        return ShaderCodeBuilder::makeCall(func, args);
    }
} // namespace trc
