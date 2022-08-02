#pragma once

#include <shared_mutex>

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>
#include <componentlib/Table.h>

#include "import/RawData.h"
#include "util/DeviceLocalDataWriter.h"
#include "AssetBaseTypes.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
#include "SharedDescriptorSet.h"

namespace trc
{
    struct TextureRegistryCreateInfo
    {
        const vkb::Device& device;
        SharedDescriptorSet::Builder& descriptorBuilder;
    };

    class TextureRegistry : public AssetRegistryModuleCacheCrtpBase<Texture>
    {
        friend class AssetHandle<Texture>;
        struct InternalStorage;

    public:
        using LocalID = TypedAssetID<Texture>::LocalID;

    public:
        explicit TextureRegistry(const TextureRegistryCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) final;

        auto add(u_ptr<AssetSource<Texture>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> TextureHandle override;

        void load(LocalID id) override;
        void unload(LocalID id) override;

    private:
        struct InternalStorage
        {
            struct Data
            {
                vkb::Image image;
                vk::UniqueImageView imageView;
            };

            ui32 deviceIndex;
            u_ptr<AssetSource<Texture>> dataSource;

            u_ptr<Data> deviceData{ nullptr };
            u_ptr<ReferenceCounter> refCounter;
        };

        using CacheItemRef = SharedCacheReference;
        template<typename T>
        using Table = componentlib::Table<T, LocalID::IndexType>;

        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size
        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 512 * 512 * 4 * 200;  // 200 512x512 images

        const vkb::Device& device;
        vkb::MemoryPool memoryPool;
        DeviceLocalDataWriter dataWriter;

        data::IdPool idPool;
        std::shared_mutex textureStorageLock;
        Table<InternalStorage> textures;

        SharedDescriptorSet::Binding descBinding;
    };

    /**
     * @brief Engine-internal representation of a texture resource
     */
    template<>
    class AssetHandle<Texture>
    {
    public:
        auto getDeviceIndex() const -> ui32;

    private:
        friend TextureRegistry;
        explicit AssetHandle(TextureRegistry::CacheItemRef r, ui32 deviceIndex);

        TextureRegistry::CacheItemRef cacheRef;
        ui32 deviceIndex;
    };
} // namespace trc
