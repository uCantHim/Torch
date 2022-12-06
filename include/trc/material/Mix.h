#pragma once

#include <vector>

#include "BasicType.h"
#include "ShaderModuleBuilder.h"

namespace trc
{
    template<ui32 N, typename T>
    class Mix : public ShaderFunction
    {
    public:
        explicit Mix()
            :
            ShaderFunction(
                "Mix",
                FunctionType{
                    { glm::vec<N, T>{}, glm::vec<N, T>{}, float{} },
                    glm::vec<N, T>{}
                }
            )
        {
        }

        void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
        {
            builder.makeReturn(builder.makeExternalCall("mix", args));
        }
    };
} // namespace trc
