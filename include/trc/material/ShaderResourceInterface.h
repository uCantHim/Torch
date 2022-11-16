#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Constant.h"
#include "ShaderCapabilities.h"
#include "ShaderCapabilityConfig.h"
#include "ShaderCodePrimitives.h"
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

        auto getDescriptorSets() const -> std::vector<std::string>;
        auto getDescriptorSetIndexPlaceholder(const std::string& setName) const
            -> std::optional<std::string>;

        auto getPushConstantSize() const -> ui32;

    private:
        friend class ShaderResourceInterface;

        std::string code;
        std::vector<ShaderInputInfo> requiredShaderInputs;

        std::vector<TextureResource> textures;
        std::unordered_map<std::string, std::string> descriptorSetIndexPlaceholders;
        ui32 pushConstantSize;
    };

    class ShaderResourceInterface
    {
    public:
        ShaderResourceInterface(const ShaderCapabilityConfig& config,
                                ShaderCodeBuilder& codeBuilder);

        auto queryTexture(TextureReference tex) -> code::Value;

        /**
         * @brief Directly query a capability
         *
         * This might throw if the capability is not suited for direct
         * access. An example is the texture sample capability, which
         * requires an additional argument to be accessed.
         */
        auto queryCapability(Capability capability) -> code::Value;

        /**
         * @brief Compile requested resources to shader code
         */
        auto compile() const -> ShaderResources;

    private:
        struct DescriptorBindingFactory
        {
            auto make(const ShaderCapabilityConfig::DescriptorBinding& binding) -> std::string;

            auto getCode() const -> std::string;
            auto getDescriptorSets() const -> std::unordered_map<std::string, std::string>;

        private:
            auto getDescriptorSetPlaceholder(const std::string& set) -> std::string;

            std::unordered_map<std::string, std::string> descriptorSetPlaceholders;
            std::stringstream generatedCode;
        };

        struct PushConstantFactory
        {
            auto make(const ShaderCapabilityConfig::PushConstant& pc) -> std::string;
            auto getCode() const -> const std::string&;

        private:
            std::string code;
        };

        struct ShaderInputFactory
        {
            auto make(Capability capability, const ShaderCapabilityConfig::ShaderInput& in)
                -> std::string;

            ui32 nextShaderInputLocation{ 0 };
            std::vector<ShaderResources::ShaderInputInfo> shaderInputs;
        };

        using Resource = const ShaderCapabilityConfig::ResourceData*;

        void requireResource(Capability capability, Resource resource);

        const ShaderCapabilityConfig& config;
        ShaderCodeBuilder* codeBuilder;

        std::unordered_set<std::string> requiredExtensions;
        std::unordered_set<util::Pathlet> requiredIncludePaths;
        std::vector<std::pair<std::string, std::optional<std::string>>> requiredMacros;

        ui32 nextSpecConstantIndex{ 0 };
        /** Vector of pairs (index, name) */
        std::vector<std::pair<ui32, std::string>> specializationConstants;
        /** Map of pairs (index -> value) */
        std::unordered_map<ui32, TextureReference> specializationConstantTextures;

        std::unordered_map<Resource, std::pair<std::string, std::string>> resourceMacros;

        DescriptorBindingFactory descriptorFactory;
        PushConstantFactory pushConstantFactory;
        ShaderInputFactory shaderInput;
    };
} // namespace trc
