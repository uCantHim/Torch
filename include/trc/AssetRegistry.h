#pragma once

#include <string>
#include <unordered_map>
#include <atomic>

#include <vkb/Image.h>

#include "Boilerplate.h"
#include "utils/Exception.h"
#include "utils/TypesafeId.h"
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
        template<typename T>
        using Ref = std::reference_wrapper<T>;

        static auto addGeometry(const NameType& key, Geometry geo)
            -> std::pair<Ref<Geometry>, ui32>;
        static auto addMaterial(const NameType& key, Material mat)
            -> std::pair<Ref<Material>, ui32>;
        static auto addImage(const NameType& key, vkb::Image img)
            -> std::pair<Ref<vkb::Image>, ui32>;

        static auto getGeometry(const NameType& key) -> Geometry&;
        static auto getMaterial(const NameType& key) -> Material&;
        static auto getImage(const NameType& key) -> vkb::Image&;

        static auto getGeometryIndex(const NameType& key) -> ui32;
        static auto getMaterialIndex(const NameType& key) -> ui32;
        static auto getImageIndex(const NameType& key) -> ui32;

    private:
        using NameToIndexMap = std::unordered_map<NameType, ui32>;

        static inline NameToIndexMap geometryNames;
        static inline NameToIndexMap materialNames;
        static inline NameToIndexMap imageNames;
    };

    class AssetRegistry
    {
    public:
        template<typename T>
        using Ref = std::reference_wrapper<T>;

        using Named = AssetRegistryNameWrapper<std::string>;

        static void init();
        static void reset();

        static auto addGeometry(Geometry geo) -> std::pair<Ref<Geometry>, GeometryID>;
        static auto addMaterial(Material mat) -> std::pair<Ref<Material>, MaterialID>;
        static auto addImage(vkb::Image img) -> std::pair<Ref<vkb::Image>, TextureID>;

        static auto getGeometry(GeometryID key) -> Geometry&;
        static auto getMaterial(MaterialID key) -> Material&;
        static auto getImage(TextureID key) -> vkb::Image&;

        static auto getDescriptorSetProvider() noexcept -> DescriptorProviderInterface&;

        static void updateMaterialBuffer();

    private:
        static inline vkb::StaticInit _init{ init, reset };

        template<typename T, typename U>
        static auto addToMap(data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
                             TypesafeID<U> key,
                             T value) -> T&;
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
