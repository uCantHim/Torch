#pragma once

#include <string>

#include "BasicType.h"
#include "MaterialFunction.h"

namespace trc
{
    template<ui32 N, typename T>
    class Mix : public MaterialFunction
    {
    public:
        explicit Mix()
            :
            MaterialFunction({
                .name="Mix",
                .inputs={
                    { "a", glm::vec<N, T>{} },
                    { "b", glm::vec<N, T>{} },
                    { "x", float{} },
                },
                .output={ "result", glm::vec<N, T>{} }
            })
        {
        }

        auto makeGlslCode(ShaderResourceInterface&) -> std::string override
        {
            return "return mix(a, b, x);";
        }
    };
} // namespace trc
