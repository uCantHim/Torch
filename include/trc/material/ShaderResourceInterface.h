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
        using ResourceID = ShaderCapabilityConfig::ResourceID;

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

        struct PushConstantInfo
        {
            ui32 offset;
            ui32 size;

            ui32 userId;
        };

        ShaderResources() = default;

        auto getGlslCode() const -> const std::string&;
        auto getRequiredShaderInputs() const
            -> const std::vector<ShaderInputInfo>&;

        /**
         * Structs { <texture>, <spec-idx> }
         *
         * The required operation at pipeline creation is:
         *
         *     specConstants[<spec-idx>] = <texture>.getDeviceIndex();
         */
        auto getRequiredTextures() const -> const std::vector<TextureResource>&;

        auto getRequiredDescriptorSets() const -> std::vector<std::string>;
        auto getDescriptorIndexPlaceholder(const std::string& setName) const
            -> std::optional<std::string>;

        /**
         * @return ui32 Total size of the stage's push constants
         */
        auto getPushConstantSize() const -> ui32;

        /**
         * @return nullopt if the resource ID does not exist or it is not
         *         associated with an active push constant value.
         */
        auto getPushConstantInfo(ResourceID resource) const -> std::optional<PushConstantInfo>;

        auto getPushConstants() const -> std::vector<PushConstantInfo>;

    private:
        friend class ShaderResourceInterface;

        std::string code;
        std::vector<ShaderInputInfo> requiredShaderInputs;

        std::vector<TextureResource> textures;
        std::unordered_map<std::string, std::string> descriptorSetIndexPlaceholders;

        std::unordered_map<ResourceID, PushConstantInfo> pushConstantInfos;
        ui32 pushConstantSize;
    };

    /**
     * This collects and compiles resources for a single shader.
     */
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

            ui32 nextNameIndex{ 0 };

            std::unordered_map<std::string, std::string> descriptorSetPlaceholders;
            std::stringstream generatedCode;
        };

        struct PushConstantFactory
        {
            using ResourceID = ShaderResources::ResourceID;
            using PushConstantInfo = ShaderResources::PushConstantInfo;

            auto make(ResourceID resource, const ShaderCapabilityConfig::PushConstant& pc)
                -> std::string;

            auto getTotalSize() const -> ui32;
            auto getInfos() const -> const std::unordered_map<ResourceID, PushConstantInfo>&;

            auto getCode() const -> std::string;

        private:
            ui32 totalSize{ 0 };
            std::unordered_map<ResourceID, PushConstantInfo> infos;

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

        void requireResource(Capability capability, ShaderCapabilityConfig::ResourceID resource);

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
