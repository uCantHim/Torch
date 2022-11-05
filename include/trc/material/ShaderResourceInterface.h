#pragma once

#include <string>
#include <unordered_map>
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

        struct ShaderInputInfo
        {
            ui32 location;
            BasicType type;
            std::string variableName;
            std::string declCode;
            Capability capability;
        };

        ShaderResources() = default;

        auto getGlslCode() const -> const std::string&;
        auto getShaderInputs() const
            -> const std::vector<ShaderInputInfo>&;

        auto getReferencedTextures() const -> const std::vector<TextureResource>&;

    private:
        friend class ShaderResourceInterface;

        std::string code;
        std::vector<ShaderInputInfo> requiredShaderInputs;

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
        using Resource = const ShaderCapabilityConfig::Resource*;

        struct TranslateResource
        {
            auto operator()(const ShaderCapabilityConfig::DescriptorBinding& binding)
                -> std::pair<std::string, std::string>;
            auto operator()(const ShaderCapabilityConfig::PushConstant& pc)
                -> std::pair<std::string, std::string>;

        private:
            auto getSetIndex(const std::string& set) -> ui32;
            auto makeBindingIndex(const std::string& set) -> ui32;

            ui32 nextSetIndex{ 0 };
            std::unordered_map<std::string, ui32> setIndices;
            std::unordered_map<std::string, ui32> bindingIndices;
        };

        struct ShaderInputFactory
        {
            auto make(Capability capability, const ShaderCapabilityConfig::ShaderInput& in)
                -> std::string;

            ui32 nextShaderInputLocation{ 0 };
            std::vector<ShaderResources::ShaderInputInfo> shaderInputs;
        };

        auto hardcoded_makeTextureAccessor(const std::string& textureIndexName) -> std::string;
        auto accessCapability(Capability capability) -> std::string;

        const ShaderCapabilityConfig& config;

        TranslateResource resourceTranslator;
        std::unordered_map<Resource, std::string> resourceCode;
        std::unordered_map<Capability, std::string> capabilityAccessors;

        ui32 nextConstantId{ 1 };
        std::unordered_map<std::string, Constant> constants;

        ui32 nextSpecConstantIndex{ 0 };
        /** Vector of pairs (index, name) */
        std::vector<std::pair<ui32, std::string>> specializationConstants;
        /** Map of pairs (index -> value) */
        std::unordered_map<ui32, TextureReference> specializationConstantTextures;

        ShaderInputFactory shaderInput;
    };
} // namespace trc
