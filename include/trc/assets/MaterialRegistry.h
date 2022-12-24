#pragma once

#include <concepts>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <trc_util/Padding.h>
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
        AssetData(ShaderModule fragModule, bool transparent);

        ShaderModule fragmentModule;

        bool transparent{ false };

        void resolveReferences(AssetManager& man);

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

    template<>
    class AssetHandle<Material>
    {
    public:
        bool isTransparent() const {
            return storage->getBaseMaterial(baseId).transparent;
        }

        auto getRuntime(MaterialSpecializationInfo params) const -> MaterialRuntime
        {
            assert(storage != nullptr);
            return storage->specialize(baseId, params);
        }

    private:
        friend class MaterialRegistry;
        AssetHandle(ui32 id, MaterialStorage& storage)
            : baseId(id), storage(&storage) {}

        ui32 baseId;
        MaterialStorage* storage;
    };

    using MaterialHandle = AssetHandle<Material>;
    using MaterialData = AssetData<Material>;
    using MaterialID = TypedAssetID<Material>;

    struct MaterialRegistryCreateInfo
    {
        const Device& device;
        SharedDescriptorSet::Builder& descriptorBuilder;

        ShaderDescriptorConfig descriptorConfig;
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
        std::mutex materialStorageLock;
        MaterialStorage storage;

        Buffer materialBuffer;

        // Descriptor
        SharedDescriptorSet::Binding descBinding;
    };
} // namespace trc
