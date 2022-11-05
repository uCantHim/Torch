#pragma once

#include <string>
#include <vector>

#include "BasicType.h"
#include "ShaderCapabilities.h"
#include "ShaderResourceInterface.h"

namespace trc
{
    struct Param
    {
        std::string name;
        BasicType type;
    };

    struct Signature
    {
        std::string name;
        std::vector<Param> inputs;
        Param output;
    };

    class MaterialFunction
    {
    public:
        explicit MaterialFunction(Signature sig);
        virtual ~MaterialFunction() = default;

        virtual auto makeGlslCode(ShaderResourceInterface& resources) -> std::string = 0;

        auto getSignature() const -> const Signature&;

    private:
        const Signature signature;
    };
} // namespace trc
