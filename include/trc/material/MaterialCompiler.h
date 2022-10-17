#pragma once

#include <cassert>

#include <string>
#include <vector>

#include "MaterialGraph.h"
#include "ShaderCapabilityConfig.h"
#include "ShaderResourceInterface.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * Contains information required to build a material pipeline
     */
    struct MaterialCompileResult
    {
        std::string fragmentGlslCode;

        /**
         * Structs { <texture>, <spec-idx> }
         *
         * The required operation at pipeline creation is:
         *
         *     specConstants[<spec-idx>] = <texture>.getDeviceIndex();
         */
        std::vector<ShaderResources::TextureResource> requiredTextures;
    };

    class MaterialCompiler
    {
    public:
        explicit MaterialCompiler(ShaderCapabilityConfig config);

        auto compile(MaterialGraph& graph) -> MaterialCompileResult;

    private:
        static auto compileFunctions(ShaderResourceInterface& resources, MaterialResultNode& mat)
            -> std::string;

        static auto call(MaterialNode* node) -> std::string;

        ShaderCapabilityConfig config;
    };
} // namespace trc
