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
        explicit MaterialFunction(Signature sig,
                                  std::vector<Capability> requiredCapabilities);
        virtual ~MaterialFunction() = default;

        virtual auto makeGlslCode(ShaderResourceInterface& resources) -> std::string = 0;

        auto getSignature() const -> const Signature&;
        auto getRequiredCapabilities() const -> const std::vector<Capability>&;

    private:
        const Signature signature;
        const std::vector<Capability> requiredCapabilities;
    };
} // namespace trc
