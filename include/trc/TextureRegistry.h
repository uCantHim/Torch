#pragma once

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>
#include <componentlib/Table.h>

#include "Assets.h"
#include "assets/RawData.h"
#include "AssetRegistryModule.h"
#include "SharedCacheReference.h"

namespace trc
{
    class TextureRegistry : public AssetRegistryModuleInterface
    {
        struct InternalStorage;

    public:
        using LocalID = TypedAssetID<Texture>::LocalID;

        /**
         * @brief Engine-internal representation of a texture resource
         */
        class Handle : public SharedCacheReference<InternalStorage>
        {
            friend TextureRegistry;
            explicit Handle(InternalStorage& s);

        public:
            auto getDeviceIndex() const -> ui32;
        };

    public:
        explicit TextureRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(const TextureData& data) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> Handle;

    private:
        struct CacheRefCounter
        {
            CacheRefCounter(LocalID tex, TextureRegistry* reg)
                : texture(tex), registry(reg)
            {}

            void inc();
            void dec();

            ui32 count{ 0 };
            LocalID texture;
            TextureRegistry* registry;
        };

        struct InternalStorage
        {
            struct Data
            {
                vkb::Image image;
                vk::UniqueImageView imageView;
            };

            inline void inc() { refCount.inc(); }
            inline void dec() { refCount.dec(); }

            ui32 deviceIndex;
            TextureData importData;  // TODO: Use AssetSource instead

            CacheRefCounter refCount;
            u_ptr<Data> deviceData{ nullptr };
        };

        template<typename T> using Table = componentlib::Table<T, LocalID::IndexType>;

        void load(LocalID id);
        void unload(LocalID id);

        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size
        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 512 * 512 * 4 * 200;  // 200 512x512 images

        const vkb::Device& device;
        const AssetRegistryModuleCreateInfo config;
        vkb::MemoryPool memoryPool;

        data::IdPool idPool;
        Table<u_ptr<InternalStorage>> textures;

        std::vector<std::pair<ui32, vk::DescriptorImageInfo>> descriptorUpdates;
        std::vector<std::pair<ui32, vk::DescriptorImageInfo>> oldDescriptorUpdates;
    };
} // namespace trc
