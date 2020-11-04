#pragma once

#include "utils/Maybe.h"

#include <string>
#include <unordered_map>
#include <atomic>

#include <vkb/Image.h>

#include "Types.h"
#include "utils/Exception.h"
#include "utils/TypesafeId.h"
#include "utils/Maybe.h"
#include "data_utils/IndexMap.h"
#include "DescriptorProvider.h"
#include "AssetIds.h"
#include "Geometry.h"
#include "Material.h"

namespace trc
{
    class DuplicateKeyError : public Exception {};
    class KeyNotFoundError : public Exception {};

    /**
     * @brief Helper that maps strings to asset indices
     */
    template<typename NameType>
    class AssetRegistryNameWrapper
    {
    public:
        static auto addGeometry(const NameType& key, Geometry geo) -> GeometryID;
        static auto addMaterial(const NameType& key, Material mat) -> MaterialID;
        static auto addImage(const NameType& key, vkb::Image img) -> TextureID;

        static auto getGeometry(const NameType& key) -> Maybe<Geometry*>;
        static auto getMaterial(const NameType& key) -> Maybe<Material*>;
        static auto getImage(const NameType& key) -> Maybe<vkb::Image*>;

        static auto getGeometryIndex(const NameType& key) -> Maybe<GeometryID>;
        static auto getMaterialIndex(const NameType& key) -> Maybe<MaterialID>;
        static auto getImageIndex(const NameType& key) -> Maybe<TextureID>;

    private:
        template<typename T>
        using NameToIndexMap = std::unordered_map<NameType, TypesafeID<T, AssetRegistryIdType>>;

        static inline NameToIndexMap<Geometry> geometryNames;
        static inline NameToIndexMap<Material> materialNames;
        static inline NameToIndexMap<vkb::Image> imageNames;
    };

    class AssetRegistry
    {
    public:
        using Named = AssetRegistryNameWrapper<std::string>;

        static void init();
        static void reset();

        static auto addGeometry(Geometry geo) -> GeometryID;
        static auto addMaterial(Material mat) -> MaterialID;
        static auto addImage(vkb::Image img) -> TextureID;

        //static auto emplaceGeometry(Geometry geo) -> GeometryID;
        //static auto emplaceMaterial(Material mat) -> MaterialID;
        //static auto emplaceImage(vkb::Image img) -> TextureID;

        static auto getGeometry(GeometryID key) -> Maybe<Geometry*>;
        static auto getMaterial(MaterialID key) -> Maybe<Material*>;
        static auto getImage(TextureID key) -> Maybe<vkb::Image*>;

        static auto getDescriptorSetProvider() noexcept -> DescriptorProviderInterface&;

        static void updateMaterials();

    private:
        static inline vkb::StaticInit _init{ init, reset };

        template<typename T, typename U, typename... Args>
        static auto addToMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                             TypesafeID<U> key,
                             Args&&... args) -> T&;
        template<typename T, typename U>
        static auto getFromMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                               TypesafeID<U> key) -> T&;

        static inline data::IndexMap<GeometryID, std::unique_ptr<Geometry>> geometries;
        static inline data::IndexMap<MaterialID, std::unique_ptr<Material>> materials;
        static inline data::IndexMap<TextureID, std::unique_ptr<vkb::Image>> images;

        static inline std::atomic<ui32> nextGeometryIndex{ 0 };
        static inline std::atomic<ui32> nextMaterialIndex{ 0 };
        static inline std::atomic<ui32> nextImageIndex{ 0 };

        //////////
        // Buffers
        static inline vkb::Buffer materialBuffer;
        static inline data::IndexMap<ui32, vk::UniqueImageView> imageViews;

        //////////////
        // Descriptors
        static constexpr ui32 MAT_BUFFER_BINDING = 0;
        static constexpr ui32 IMG_DESCRIPTOR_BINDING = 1;

        static void createDescriptors();
        static void updateDescriptors();

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vk::UniqueDescriptorSet descSet;
        static inline DescriptorProvider descriptorProvider{ {}, {} };
    };
} // namespace trc

#include "AssetRegistry.inl"
