#pragma once

#include <string>
#include <unordered_map>
#include <atomic>

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>

#include "Types.h"
#include "trc_util/Exception.h"
#include "trc_util/data/IndexMap.h"
#include "trc_util/functional/Maybe.h"
#include "core/DescriptorProvider.h"
#include "AssetIds.h"
#include "assets/RawData.h"
#include "Geometry.h"
#include "Texture.h"
#include "Material.h"
#include "Rig.h"
#include "AnimationDataStorage.h"
#include "text/FontDataStorage.h"

namespace trc
{
    class Instance;
    class AssetRegistry;

    class DuplicateKeyError : public Exception {};
    class KeyNotFoundError : public Exception {};

    struct AssetRegistryCreateInfo
    {
        vk::BufferUsageFlags geometryBufferUsage{};

        vk::ShaderStageFlags materialDescriptorStages{};
        vk::ShaderStageFlags textureDescriptorStages{};
        vk::ShaderStageFlags geometryDescriptorStages{};

        bool enableRayTracing{ true };
    };

    class AssetRegistry
    {
    public:
        explicit AssetRegistry(const Instance& instance,
                               const AssetRegistryCreateInfo& info = {});

        auto add(const GeometryData& geo,
                 std::optional<RigData> rigData = std::nullopt) -> GeometryID;
        auto add(MaterialDeviceHandle mat) -> MaterialID;
        auto add(const TextureData& tex) -> TextureID;

        auto get(GeometryID key) -> GeometryDeviceHandle;
        auto get(MaterialID key) -> MaterialDeviceHandle&;
        auto get(TextureID key) -> TextureDeviceHandle;

        auto getFonts() -> FontDataStorage&;
        auto getFonts() const -> const FontDataStorage&;
        auto getAnimations() -> AnimationDataStorage&;
        auto getAnimations() const -> const AnimationDataStorage&;

        auto getDescriptorSetProvider() const noexcept -> const DescriptorProviderInterface&;

        // TODO: Don't re-upload ALL materials every time one is added
        void updateMaterials();

    private:
        static auto addDefaultValues(const AssetRegistryCreateInfo& info) -> AssetRegistryCreateInfo;

        template<typename T, typename U, typename... Args>
        auto addToMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                             TypesafeID<U> key,
                             Args&&... args) -> T&;
        template<typename T, typename U>
        auto getFromMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                               TypesafeID<U> key) -> T&;

        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 200000000;  // 200 MiB
        static constexpr ui32 MATERIAL_BUFFER_DEFAULT_SIZE = sizeof(MaterialDeviceHandle) * 100;
        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size
        static constexpr ui32 MAX_GEOMETRY_COUNT = 5000;

        const Instance& instance;
        const vkb::Device& device;
        vkb::MemoryPool memoryPool;

        const AssetRegistryCreateInfo config;

        /**
         * GPU resources for geometry data
         */
        struct GeometryStorage
        {
            operator GeometryDeviceHandle()
            {
                return {
                    *indexBuf, numIndices, vk::IndexType::eUint32,
                    *vertexBuf, numVertices,
                    rig.has_value() ? &rig.value() : nullptr
                };
            }

            vkb::DeviceLocalBuffer indexBuf;
            vkb::DeviceLocalBuffer vertexBuf;

            ui32 numIndices{ 0 };
            ui32 numVertices{ 0 };

            std::optional<Rig> rig;
        };

        struct TextureStorage
        {
            operator TextureDeviceHandle() {
                return {};
            }

            vkb::Image image;
            vk::UniqueImageView imageView;
        };

        data::IndexMap<GeometryID::ID, u_ptr<GeometryStorage>> geometries;
        data::IndexMap<MaterialID::ID, u_ptr<Material>> materials;
        data::IndexMap<TextureID::ID, u_ptr<TextureStorage>> textures;

        std::atomic<ui32> nextGeometryIndex{ 0 };
        std::atomic<ui32> nextMaterialIndex{ 0 };
        std::atomic<ui32> nextImageIndex{ 0 };

        //////////
        // Buffers
        vkb::Buffer materialBuffer;

        //////////////
        // Descriptors
        enum DescBinding
        {
            eMaterials = 0,
            eTextures = 1,
            eVertexBuffers = 2,
            eIndexBuffers = 3,
        };

        void createDescriptors();
        void writeDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider descriptorProvider{ {}, {} };

        ////////////////////////////
        // Additional asset storages
        FontDataStorage fontData;
        AnimationDataStorage animationStorage;
    };
} // namespace trc

#include "AssetRegistry.inl"
