#pragma once

#include <string>
#include <unordered_map>
#include <atomic>

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <nc/data/IndexMap.h>
#include <nc/functional/Maybe.h>

#include "Types.h"
#include "utils/Exception.h"
#include "DescriptorProvider.h"
#include "AssetIds.h"
#include "Geometry.h"
#include "Material.h"
#include "AnimationDataStorage.h"
#include "text/FontDataStorage.h"

namespace trc
{
    class Instance;
    class AssetRegistry;

    class DuplicateKeyError : public Exception {};
    class KeyNotFoundError : public Exception {};

    /**
     * @brief Helper that maps strings to asset indices
     */
    template<typename NameType>
    class AssetRegistryNameWrapper
    {
    public:
        explicit AssetRegistryNameWrapper(AssetRegistry& ar);

        auto add(const NameType& key, GeometryData geo) -> GeometryID;
        auto add(const NameType& key, Material mat) -> MaterialID;
        auto add(const NameType& key, vkb::Image img) -> TextureID;

        auto getGeo(const NameType& key) -> Maybe<Geometry>;
        auto getMat(const NameType& key) -> Maybe<Material&>;
        auto getTex(const NameType& key) -> Maybe<Texture>;

        auto getGeoIndex(const NameType& key) -> Maybe<GeometryID>;
        auto getMatIndex(const NameType& key) -> Maybe<MaterialID>;
        auto getTexIndex(const NameType& key) -> Maybe<TextureID>;

    private:
        template<typename T>
        using NameToIndexMap = std::unordered_map<NameType, TypesafeID<T, AssetIdType>>;

        AssetRegistry* ar;

        NameToIndexMap<Geometry> geometryNames;
        NameToIndexMap<Material> materialNames;
        NameToIndexMap<vkb::Image> imageNames;
    };

    class AssetRegistry
    {
    public:
        explicit AssetRegistry(const Instance& instance);

        auto add(const GeometryData& geo,
                 std::optional<RigData> rigData = std::nullopt) -> GeometryID;
        auto add(Material mat) -> MaterialID;
        auto add(vkb::Image image) -> TextureID;

        auto get(GeometryID key) -> Geometry;
        auto get(MaterialID key) -> Material&;
        auto get(TextureID key) -> Texture;

        auto getFonts() -> FontDataStorage&;
        auto getFonts() const -> const FontDataStorage&;
        auto getAnimations() -> AnimationDataStorage&;
        auto getAnimations() const -> const AnimationDataStorage&;

        auto getDescriptorSetProvider() const noexcept -> const DescriptorProviderInterface&;

        // TODO: Don't re-upload ALL materials every time one is added
        void updateMaterials();

    public:
        AssetRegistryNameWrapper<std::string> named{ *this };

    private:
        template<typename T, typename U, typename... Args>
        auto addToMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                             TypesafeID<U> key,
                             Args&&... args) -> T&;
        template<typename T, typename U>
        auto getFromMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                               TypesafeID<U> key) -> T&;

        /**
         * GPU resources for geometry data
         */
        struct GeometryStorage
        {
            operator Geometry()
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
            operator Texture() {
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

        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 200000000;  // 200 MiB
        static constexpr ui32 MATERIAL_BUFFER_DEFAULT_SIZE = sizeof(Material) * 100;
        static constexpr ui32 MAX_TEXTURE_COUNT = 2000;  // For static descriptor size

        const Instance& instance;
        const vkb::Device& device;
        vkb::MemoryPool memoryPool;

        //////////
        // Buffers
        vkb::Buffer materialBuffer;

        //////////////
        // Descriptors
        static constexpr ui32 MAT_BUFFER_BINDING = 0;
        static constexpr ui32 IMG_DESCRIPTOR_BINDING = 1;

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



    template<typename Derived>
    inline AssetID<Derived>::AssetID(ui32 id, AssetRegistry& ar)
        :
        TypesafeID<Derived, AssetIdType>(id),
        ar(&ar)
    {}

    template<typename Derived>
    inline auto AssetID<Derived>::id() const -> AssetIdType
    {
        return static_cast<AssetIdType>(*this);
    }

    template<typename Derived>
    inline auto AssetID<Derived>::get()
    {
        assert(ar != nullptr);

        return ar->get(*this);
    }

    template<typename Derived>
    inline auto AssetID<Derived>::getAssetRegistry() -> AssetRegistry&
    {
        assert(ar != nullptr);

        return *ar;
    }
} // namespace trc

#include "AssetRegistry.inl"
