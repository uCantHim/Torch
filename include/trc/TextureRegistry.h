#pragma once

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>
#include <componentlib/Table.h>

#include "AssetIds.h"
#include "Texture.h"
#include "AssetRegistryModule.h"

namespace trc
{
    class TextureRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = TextureID::LocalID;
        using Handle = TextureDeviceHandle;

        explicit TextureRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(const TextureData& data) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> TextureDeviceHandle;

    private:
        struct InternalStorage
        {
            operator TextureDeviceHandle() {
                return {};
            }

            vkb::Image image;
            vk::UniqueImageView imageView;
        };

        template<typename T> using Table = componentlib::Table<T, LocalID::Type>;

        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size
        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 512 * 512 * 4 * 200;  // 200 512x512 images

        const vkb::Device& device;
        const AssetRegistryModuleCreateInfo config;
        vkb::MemoryPool memoryPool;

        data::IdPool idPool;
        Table<InternalStorage> textures;

        Table<vk::DescriptorImageInfo> descImageInfos;
    };
} // namespace trc
