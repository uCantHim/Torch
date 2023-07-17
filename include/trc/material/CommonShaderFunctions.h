#pragma once

#include <vector>

#include "BasicType.h"
#include "FragmentShader.h"
#include "ShaderModuleBuilder.h"

namespace trc
{
    class TangentToWorldspace : public ShaderFunction
    {
    public:
        TangentToWorldspace();
        void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override;
    };

    template<ui32 N, typename T>
    class Mix : public ShaderFunction
    {
    public:
        Mix();

        void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override {
            builder.makeReturn(builder.makeExternalCall("mix", args));
        }
    };

    class TextureSample : public ShaderFunction
    {
    public:
        TextureSample()
            : ShaderFunction("sampleTexture2D", FunctionType{ { ui32{}, vec2{} }, vec4{} })
        {}

        void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
        {
            auto textures = builder.makeCapabilityAccess(FragmentCapability::kTextureSample);
            builder.makeReturn(
                builder.makeExternalCall(
                    "texture",
                    { builder.makeArrayAccess(textures, args.at(0)), args.at(1) }
                )
            );
        }
    };

    template<ui32 N, typename T>
    Mix<N, T>::Mix()
        :
        ShaderFunction(
            "Mix",
            FunctionType{
                { glm::vec<N, T>{}, glm::vec<N, T>{}, float{} },
                glm::vec<N, T>{}
            }
        )
    {}
} // namespace trc
