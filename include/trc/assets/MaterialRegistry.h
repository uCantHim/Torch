#pragma once

#include <concepts>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <trc_util/Padding.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "trc/Types.h"
#include "trc/assets/AssetManagerInterface.h"
#include "trc/assets/AssetReference.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/TextureRegistry.h"
#include "trc/base/Buffer.h"

#include "trc/material/MaterialStorage.h"

namespace trc
{
    class MaterialRegistry;

    struct Material
    {
        using Registry = MaterialRegistry;
    };

    template<>
    struct AssetData<Material>
    {
        // v old data
        vec3 color{ 0.0f, 0.0f, 0.0f };

        float specularCoefficient{ 1.0f };
        float roughness{ 1.0f };
        float metallicness{ 0.0f };

        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        bool doPerformLighting{ true };

        AssetReference<Texture> albedoTexture{};
        AssetReference<Texture> normalTexture{};
        // ^ old data

        std::optional<MaterialInfo> createInfo{ std::nullopt };

        void resolveReferences(AssetManager& man);

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

    template<>
    class AssetHandle<Material>
    {
    public:
        bool isTransparent() const {
            return storage->getFragmentParams(baseId).transparent;
        }

        auto getRuntime(PipelineVertexParams params) -> MaterialRuntimeInfo&
        {
            assert(storage != nullptr);
            return storage->getRuntime(baseId, params);
        }

        auto getRuntime(PipelineVertexParams params) const -> const MaterialRuntimeInfo&
        {
            assert(storage != nullptr);
            return storage->getRuntime(baseId, params);
        }

    private:
        friend class MaterialRegistry;
        AssetHandle(MatID id, MaterialStorage& storage)
            : baseId(id), storage(&storage) {}

        MatID baseId;
        MaterialStorage* storage;
    };

    using MaterialHandle = AssetHandle<Material>;
    using MaterialData = AssetData<Material>;
    using MaterialID = TypedAssetID<Material>;

    struct MaterialRegistryCreateInfo
    {
        const Device& device;
        SharedDescriptorSet::Builder& descriptorBuilder;
    };

    class MaterialRegistry : public AssetRegistryModuleInterface<Material>
    {
    public:
        using LocalID = TypedAssetID<Material>::LocalID;

        explicit MaterialRegistry(const MaterialRegistryCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) final;

        auto add(u_ptr<AssetSource<Material>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> MaterialHandle override;

    private:
        data::IdPool idPool;
        std::unordered_map<LocalID, MatID> materialIds;

        std::mutex materialStorageLock;
        MaterialStorage storage;
        Buffer materialBuffer;

        // Descriptor
        SharedDescriptorSet::Binding descBinding;
    };
} // namespace trc
