#pragma once

#include <unordered_map>

#include "Boilerplate.h"
#include "Asset.h"
#include "Geometry.h"
#include "Material.h"
#include "data_utils/IndexMap.h"

namespace trc
{
    template<typename T>
    class AssetRegistry
    {
    public:
        static_assert(std::is_base_of_v<Asset, T>,
                      "Types stored in the asset registry must be derived from Asset");
        static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>, "");

        static auto add(Asset::ID id, T obj) -> T&
        {
            assert(assets[id] == nullptr);

            auto& result = *assets.emplace(id, new T(std::move(obj)));
            result.id = id;

            return result;
        }

        template<typename ...Args>
        static auto emplace(Asset::ID id, Args&&... args) -> T&
        {
            assert(assets[id] == nullptr);

            auto& result = *assets.emplace(id, new T(std::forward<Args>(args)...));
            result.id = id;

            return result;
        }

        static auto get(Asset::ID id) -> T&
        {
            assert(assets.size() > id && assets[id] != nullptr);

            return *assets[id];
        }

    private:
        static inline data::IndexMap<Asset::ID, std::unique_ptr<T>> assets;
    };

    /**
     * @brief Asset registry for geometries
     */
    class GeometryRegistry : public AssetRegistry<Geometry>
    {
    public:
    };

    class MaterialRegistry
    {

    };
} // namespace trc
