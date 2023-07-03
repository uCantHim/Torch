#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "trc/assets/AssetType.h"

namespace trc
{
    template<typename U>
    class AssetTypeMap
    {
    public:
        using value_type = U;
        using reference = U&;
        using const_reference = const U&;

        template<AssetBaseType T>
        inline auto at() -> reference
        {
            return at(AssetType::make<T>());
        }

        inline auto at(const AssetType& type) -> reference
        {
            std::shared_lock lock(mutex);
            return values.at(type.getName());
        }

        template<AssetBaseType T, typename ...Args>
            requires std::constructible_from<value_type, Args...>
        auto try_emplace(Args&&... args) -> std::pair<reference, bool>;

        template<typename ...Args>
            requires std::constructible_from<value_type, Args...>
        auto try_emplace(const AssetType& type, Args&&... args) -> std::pair<reference, bool>;

    private:
        std::shared_mutex mutex;
        std::unordered_map<std::string, value_type> values;
    };

    template<typename U>
    template<AssetBaseType T, typename ...Args>
        requires std::constructible_from<U, Args...>
    auto AssetTypeMap<U>::try_emplace(Args&&... args) -> std::pair<reference, bool>
    {
        return try_emplace(AssetType::make<T>(), std::forward<Args>(args)...);
    }

    template<typename U>
    template<typename ...Args>
        requires std::constructible_from<U, Args...>
    auto AssetTypeMap<U>::try_emplace(const AssetType& type, Args&&... args) -> std::pair<reference, bool>
    {
        const auto& key = type.getName();

        std::scoped_lock lock(mutex);
        auto [it, success] = values.try_emplace(key, std::forward<Args>(args)...);
        return { it->second, success };
    }
} // namespace trc
