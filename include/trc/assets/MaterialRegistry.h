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
#include "trc/material/MaterialSpecialization.h"

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
        AssetData() = default;
        AssetData(ShaderModule fragModule, bool transparent);

        std::unordered_map<MaterialKey, MaterialProgramData> programs;
        bool transparent{ false };

        void resolveReferences(AssetManager& man);

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
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
        friend Handle;

        struct Storage
        {
            auto getSpecialization(const MaterialKey& key) const -> MaterialRuntime
            {
                assert(runtimePrograms.at(key.flags.toIndex()) != nullptr);
                return runtimePrograms.at(key.flags.toIndex())->makeRuntime();
            }

            MaterialData data;
            std::array<
                u_ptr<const MaterialShaderProgram>,
                MaterialKey::MaterialSpecializationFlags::size()
            > runtimePrograms;
        };

        const ShaderDescriptorConfig descriptorConfig;

        std::mutex materialStorageLock;
        data::IdPool localIdPool;
        data::IndexMap<LocalID, u_ptr<Storage>> storage;

        Buffer materialBuffer;

        // Descriptor
        SharedDescriptorSet::Binding descBinding;
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

        auto getRuntime(MaterialSpecializationInfo params) const -> MaterialRuntime
        {
            assert(storage != nullptr);
            return storage->getSpecialization(params);
        }

    private:
        friend class MaterialRegistry;
        AssetHandle(MaterialRegistry::Storage& storage)
            : storage(&storage) {}

        MaterialRegistry::Storage* storage;
    };
} // namespace trc
