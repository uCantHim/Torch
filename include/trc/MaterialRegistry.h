#pragma once

#include <vkb/Buffer.h>
#include <trc_util/Padding.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "Assets.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
#include "assets/RawData.h"
#include "Texture.h"

namespace trc
{
    class MaterialRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = TypedAssetID<Material>::LocalID;

        struct Handle
        {
        public:
            auto getBufferIndex() const -> ui32 {
                return id;
            }

        private:
            friend class MaterialRegistry;
            explicit Handle(ui32 bufferIndex) : id(bufferIndex) {}

            ui32 id;
        };

        MaterialRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(u_ptr<AssetSource<Material>> source) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> Handle;

        template<std::invocable<MaterialData&> F>
        void modify(LocalID id, F&& func)
        {
            func(materials.at(id)->matData);
            // Material buffer needs updating now
        }

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

        const AssetRegistryModuleCreateInfo config;

        data::IdPool idPool;
        data::IndexMap<LocalID::IndexType, u_ptr<InternalStorage>> materials;
        vkb::Buffer materialBuffer;

        vk::DescriptorBufferInfo matBufferDescInfo;
    };
} // namespace trc
