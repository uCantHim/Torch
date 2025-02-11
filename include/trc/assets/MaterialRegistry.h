#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include <trc_util/Padding.h>
#include <trc_util/data/IdPool.h>
#include <trc_util/data/SafeVector.h>

#include "trc/Types.h"
#include "trc/assets/AssetReference.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/TextureRegistry.h"
#include "trc/material/MaterialProgram.h"
#include "trc/material/MaterialSpecialization.h"

namespace trc
{
    class MaterialRegistry;

    struct Material
    {
        using Registry = MaterialRegistry;

        static consteval auto name() -> std::string_view {
            return "torch_mat";
        }
    };

    template<>
    struct AssetData<Material>
    {
        /**
         * @brief Create a material
         */
        explicit AssetData(const MaterialBaseInfo& createInfo);

        MaterialSpecializationCache shaderProgram;

        /**
         * Default values for runtime parameters to the material's shader
         * program. Runtime parameters are usually implemented as push constants.
         */
        std::vector<std::pair<ui32, std::vector<std::byte>>> runtimeValueDefaults;

        bool transparent{ false };
        std::optional<vk::PolygonMode> polygonMode;
        std::optional<float> lineWidth;
        std::optional<vk::CullModeFlags> cullMode;
        std::optional<vk::FrontFace> frontFace;

        std::optional<bool> depthWrite;
        std::optional<bool> depthTest;
        std::optional<float> depthBiasConstantFactor;
        std::optional<float> depthBiasSlopeFactor;

        void resolveReferences(AssetManager& man);

        static void serialize(const AssetData<Material>& data, std::ostream& os);
        static auto deserialize(std::istream& is)
            -> std::expected<AssetData<Material>, AssetParseError>;

    private:
        /**
         * Implements all types of runtime constants that Torch defines.
         */
        struct RuntimeConstantDeserializer : public shader::ShaderRuntimeConstantDeserializer
        {
            auto deserialize(const std::string& data)
                -> std::optional<s_ptr<shader::ShaderRuntimeConstant>> override;

            // The output. After calling `deserialize` an arbitrary number of
            // times, this array will contain all texture references that were
            // parsed, if any.
            std::vector<AssetReference<Texture>> loadedTextures;
        };

        /** Used internally during serialization. */
        explicit AssetData(MaterialSpecializationCache specializations);

        std::vector<AssetReference<Texture>> requiredTextures;
    };

    using MaterialHandle = AssetHandle<Material>;
    using MaterialData = AssetData<Material>;
    using MaterialID = TypedAssetID<Material>;

    template<>
    struct AssetSerializerTraits<Material>
    {
        static void serialize(const MaterialData& data, std::ostream& os);
        static auto deserialize(std::istream& is) -> AssetParseResult<Material>;
    };

    /**
     * @brief Create a material by specifying base information manually.
     *
     * This is the most detailed possible way to create a material. Usually,
     * this function will be used internally by tools.
     *
     * @param baseInfo The minimum required information to create a material.
     *                 Includes a fragment shader, meaning a description of the
     *                 material's lighting computations.
     */
    auto makeMaterial(const MaterialBaseInfo& baseInfo) -> MaterialData;

    /**
     * @brief Specialize a material description to create a runtime program.
     *
     * @param data           The material description.
     * @param specialization Specialization info.
     */
    auto makeMaterialProgram(MaterialData& data,
                             const MaterialSpecializationInfo& specialization)
        -> std::expected<u_ptr<MaterialProgram>, ShaderCompileError>;

    class MaterialRegistry : public AssetRegistryModuleInterface<Material>
    {
    public:
        using LocalID = TypedAssetID<Material>::LocalID;

        MaterialRegistry() = default;

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) final;

        auto add(u_ptr<AssetSource<Material>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> MaterialHandle override;

    private:
        friend Handle;

        /**
         * @brief Manages specializations for a material.
         *
         * Lazily creates material specializations (shader programs and
         * runtimes) when they're requested.
         */
        struct SpecializationStorage
        {
            auto getSpecialization(const MaterialKey& key) -> s_ptr<MaterialRuntime>;

            static constexpr size_t kNumSpecializations
                = MaterialKey::MaterialSpecializationFlags::size();

            MaterialData data;
            std::array<u_ptr<const MaterialProgram>, kNumSpecializations> shaderPrograms;
            std::array<s_ptr<MaterialRuntime>, kNumSpecializations> runtimes;
        };

        data::IdPool<ui64> localIdPool;
        util::SafeVector<SpecializationStorage> storage;
    };

    template<>
    class AssetHandle<Material>
    {
    public:
        bool isTransparent() const
        {
            assert(storage != nullptr);
            return storage->data.transparent;
        }

        auto getRuntime(MaterialSpecializationInfo params) const -> s_ptr<MaterialRuntime>
        {
            assert(storage != nullptr);
            return storage->getSpecialization(params);
        }

    private:
        friend class MaterialRegistry;
        AssetHandle(MaterialRegistry::SpecializationStorage& storage)
            : storage(&storage) {}

        MaterialRegistry::SpecializationStorage* storage;
    };
} // namespace trc
