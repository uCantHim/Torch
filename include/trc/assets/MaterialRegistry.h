#pragma once

#include <unordered_set>

#include <vkb/Buffer.h>
#include <trc_util/Padding.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "AssetReference.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
#include "TextureRegistry.h"

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
        vec3 color{ 0.0f, 0.0f, 0.0f };

        vec4 ambientKoefficient{ 1.0f };
        vec4 diffuseKoefficient{ 1.0f };
        vec4 specularKoefficient{ 1.0f };

        float shininess{ 1.0f };
        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        bool doPerformLighting{ true };

        AssetReference<Texture> albedoTexture{};
        AssetReference<Texture> normalTexture{};
    };

    template<>
    class AssetHandle<Material>
    {
    public:
        auto getBufferIndex() const -> ui32 {
            return id;
        }

    private:
        friend class MaterialRegistry;
        explicit AssetHandle(ui32 bufferIndex) : id(bufferIndex) {}

        ui32 id;
    };

    using MaterialHandle = AssetHandle<Material>;
    using MaterialData = AssetData<Material>;
    using MaterialID = TypedAssetID<Material>;

    struct MaterialRegistryCreateInfo
    {
        const vkb::Device& device;
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

        auto getData(LocalID id) -> MaterialData;
        template<std::invocable<MaterialData&> F>
        void modify(LocalID id, F&& func);

    private:
        static constexpr ui32 NO_TEXTURE = UINT32_MAX;

        struct InternalStorage
        {
            ui32 bufferIndex;
            MaterialData matData;
            std::optional<AssetHandle<Texture>> albedoTex;
            std::optional<AssetHandle<Texture>> normalTex;
        };

        struct MaterialDeviceData
        {
            MaterialDeviceData() = default;
            MaterialDeviceData(const InternalStorage& data);

            vec4 color{ 0.0f, 0.0f, 0.0f, 1.0f };

            vec4 kAmbient{ 1.0f };
            vec4 kDiffuse{ 1.0f };
            vec4 kSpecular{ 1.0f };

            float shininess{ 1.0f };
            float reflectivity{ 0.0f };

            ui32 diffuseTexture{ NO_TEXTURE };
            ui32 specularTexture{ NO_TEXTURE };
            ui32 bumpTexture{ NO_TEXTURE };

            bool32 performLighting{ true };

            ui32 __padding[2]{ 0, 0 };
        };

        static_assert(util::sizeof_pad_16_v<MaterialDeviceData> == sizeof(MaterialDeviceData),
                      "MaterialDeviceData struct must be padded to 16 bytes for std430!");

        static constexpr ui32 MATERIAL_BUFFER_DEFAULT_SIZE = sizeof(MaterialDeviceData) * 100;

        data::IdPool idPool;

        // Host and device data storage
        std::mutex materialStorageLock;
        std::vector<u_ptr<InternalStorage>> materials;
        vkb::Buffer materialBuffer;

        // Descriptor
        SharedDescriptorSet::Binding descBinding;

        // Device data updates
        std::mutex changedMaterialsLock;
        std::unordered_set<LocalID> changedMaterials;
    };


    template<std::invocable<MaterialData&> F>
    void MaterialRegistry::modify(const LocalID id, F&& func)
    {
        {
            std::scoped_lock lock(materialStorageLock);
            func(materials.at(id)->matData);
        }
        {
            std::scoped_lock lock(changedMaterialsLock);
            changedMaterials.emplace(id);
        }
    }
} // namespace trc
