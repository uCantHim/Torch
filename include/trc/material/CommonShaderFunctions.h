#pragma once

#include <vector>

#include "BasicType.h"
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
