#pragma once

#include <istream>
#include <ostream>

#include "MaterialRegistry.h"

namespace trc
{
    class SimpleMaterialRegistry;

    struct SimpleMaterial
    {
        using Registry = SimpleMaterialRegistry;
    };

    /**
     * Used for a simplified material creation process as well as for
     * material imports.
     */
    template<>
    struct AssetData<SimpleMaterial>
    {
        vec3 color{ 1.0f };

        float specularCoefficient{ 1.0f };
        float roughness{ 1.0f };
        float metallicness{ 0.0f };

        /**
         * To enable transparency with an albedo texture with alpha
         * channel, set the opacity to something smaller than 1.
         */
        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        bool emissive{ false };

        AssetReference<Texture> albedoTexture{};
        AssetReference<Texture> normalTexture{};

        void resolveReferences(AssetManager& man);

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

    using SimpleMaterialData = AssetData<SimpleMaterial>;

    /**
     * @brief Create a material from SimpleMaterialData
     */
    auto makeMaterial(const SimpleMaterialData& data) -> MaterialData;

    template<>
    class AssetHandle<SimpleMaterial>
    {
    public:
        inline auto getMaterial() -> MaterialID {
            return baseMat;
        }

    private:
        friend class SimpleMaterialRegistry;
        explicit AssetHandle(MaterialID baseMat) : baseMat(baseMat) {}

        MaterialID baseMat;
    };

    class SimpleMaterialRegistry : public AssetRegistryModuleInterface<SimpleMaterial>
    {
    public:
        explicit SimpleMaterialRegistry(AssetManager& assetManager);

        void update(vk::CommandBuffer, FrameRenderState&) override {};

        auto add(u_ptr<AssetSource<SimpleMaterial>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> Handle override;

    private:
        AssetManager* assetManager;
        MaterialID baseMaterialId;

        data::IdPool localIdPool;
        std::unordered_map<LocalID, MaterialID> baseMaterials;
    };
} // namespace trc
