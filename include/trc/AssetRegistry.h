#pragma once

#include <string>
#include <unordered_map>

#include "Boilerplate.h"
#include "utils/Exception.h"
#include "data_utils/IndexMap.h"
#include "Geometry.h"
#include "Material.h"

namespace trc
{
    class DuplicateKeyError : public Exception {};
    class KeyNotFoundError : public Exception {};

    class AssetRegistry
    {
    public:
        static void init();

        static auto addGeometry(ui32 key, Geometry geo) -> Geometry&;
        static auto addMaterial(ui32 key, Material mat) -> Material&;

        static auto getGeometry(ui32 key) -> Geometry&;
        static auto getMaterial(ui32 key) -> Material&;

        static auto getDescriptorSetLayout() noexcept -> vk::DescriptorSetLayout;
        static auto getDescriptorSet() noexcept -> vk::DescriptorSet;

    private:
        static inline bool __init = []() {
            vkb::VulkanBase::onInit(AssetRegistry::init);
            return true;
        }();

        template<typename T>
        using StrMap = std::unordered_map<std::string, T>;

        template<typename T>
        static auto addToMap(data::IndexMap<ui32, std::unique_ptr<T>>& map, ui32 key, T value) -> T&;
        template<typename T>
        static auto getFromMap(data::IndexMap<ui32, std::unique_ptr<T>>& map, ui32 key) -> T&;

        static inline data::IndexMap<ui32, std::unique_ptr<Geometry>> geometries;
        static inline data::IndexMap<ui32, std::unique_ptr<Material>> materials;

        //////////
        // Buffers
        static void updateMaterialBuffer();

        static inline vkb::DeviceLocalBuffer materialBuffer;

        //////////////
        // Descriptors
        static constexpr ui32 MAT_BUFFER_BINDING = 0;

        static void createDescriptors();
        static void updateDescriptors();

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vk::UniqueDescriptorSet descSet;
    };



    template<typename T>
    auto AssetRegistry::addToMap(
        data::IndexMap<ui32, std::unique_ptr<T>>& map,
        ui32 key, T value) -> T&
    {
        static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>, "");
        assert(key != UINT32_MAX);  // Reserved ID that signals empty value

        if (map[key] != nullptr) {
            throw DuplicateKeyError();
        }

        return *map.emplace(key, std::make_unique<T>(std::move(value)));
    }

    template<typename T>
    auto AssetRegistry::getFromMap(data::IndexMap<ui32, std::unique_ptr<T>>& map, ui32 key) -> T&
    {
        assert(key != UINT32_MAX);  // Reserved ID that signals empty value

        if (map[key] == nullptr) {
            throw KeyNotFoundError();
        }

        return *map[key];
    }
} // namespace trc
