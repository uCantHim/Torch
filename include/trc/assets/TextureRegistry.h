#pragma once

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>
#include <componentlib/Table.h>

#include "AssetBaseTypes.h"
#include "import/RawData.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"

namespace trc
{
    class TextureRegistry : public AssetRegistryModuleCacheCrtpBase<Texture>
    {
        struct InternalStorage;

    public:
        using LocalID = TypedAssetID<Texture>::LocalID;

        /**
         * @brief Engine-internal representation of a texture resource
         */
        class Handle : public SharedCacheReference<InternalStorage>
        {
        public:
            auto getDeviceIndex() const -> ui32;

        private:
            friend TextureRegistry;
            explicit Handle(InternalStorage& s);
        };

    public:
        explicit TextureRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto add(u_ptr<AssetSource<Texture>> source) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> Handle;

        void load(LocalID id) override;
        void unload(LocalID id) override;

    private:
        struct InternalStorage : CacheRefCounter
        {
            struct Data
            {
                vkb::Image image;
                vk::UniqueImageView imageView;
            };

            ui32 deviceIndex;
            u_ptr<AssetSource<Texture>> dataSource;

            u_ptr<Data> deviceData{ nullptr };
        };

        template<typename T> using Table = componentlib::Table<T, LocalID::IndexType>;

        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size
        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 512 * 512 * 4 * 200;  // 200 512x512 images

        const vkb::Device& device;
        const AssetRegistryModuleCreateInfo config;
        vkb::MemoryPool memoryPool;

        data::IdPool idPool;
        Table<u_ptr<InternalStorage>> textures;

        SharedDescriptorSet::Binding descBinding;
    };
} // namespace trc
