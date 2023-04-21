#pragma once

#include <concepts>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include <trc_util/Padding.h>
#include <trc_util/data/IdPool.h>
#include <trc_util/data/SafeVector.h>

#include "trc/Types.h"
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

        static consteval auto name() -> std::string_view {
            return "torch_mat";
        }
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

        data::IdPool<ui64> localIdPool;
        util::SafeVector<Storage> storage;
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
