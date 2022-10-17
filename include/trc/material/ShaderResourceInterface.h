#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Constant.h"
#include "ShaderCapabilities.h"
#include "ShaderCapabilityConfig.h"
#include "trc/assets/AssetReference.h"
#include "trc/assets/Texture.h"

namespace trc
{
    struct TextureReference
    {
        AssetReference<Texture> texture;
    };

    class ShaderResources
    {
    public:
        struct TextureResource
        {
            TextureReference ref;
            ui32 specializationConstantIndex;
        };

        ShaderResources() = default;

        auto getGlslCode() const -> const std::string&;
        auto getReferencedTextures() const -> const std::vector<TextureResource>&;

    private:
        friend class ShaderResourceInterface;

        std::string code;
        std::vector<TextureResource> textures;
    };

    class ShaderResourceInterface
    {
    public:
        explicit ShaderResourceInterface(const ShaderCapabilityConfig& config);

        auto makeScalarConstant(Constant constantValue) -> std::string;
        auto queryConstant(Builtin constant) -> std::string;
        auto queryTexture(TextureReference tex) -> std::string;

        auto getConstantType(Builtin constant) -> BasicType;

        /**
         * @brief Compile requested resources to shader code
         */
        auto compile() const -> ShaderResources;

    private:
        auto hardcoded_makeTextureAccessor(const std::string& textureIndexName) -> std::string;
        auto accessCapability(Capability capability) -> std::string;

        const ShaderCapabilityConfig& config;
        std::unordered_map<Capability, std::string> capabilityCode;
        std::unordered_map<Capability, std::string> accessors;

        ui32 nextConstantId{ 1 };
        std::unordered_map<std::string, Constant> constants;

        ui32 nextSpecConstantIndex{ 0 };
        /** Vector of pairs (index, name) */
        std::vector<std::pair<ui32, std::string>> specializationConstants;
        /** Map of pairs (index -> value) */
        std::unordered_map<ui32, TextureReference> specializationConstantTextures;
    };
} // namespace trc
