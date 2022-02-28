#include "AssetRegistry.h"



template<typename T, typename U, typename... Args>
auto trc::AssetRegistry::addToMap(
    data::IndexMap<TypesafeID<U>, u_ptr<T>>& map,
    TypesafeID<U> key,
    Args&&... args) -> T&
{
    if (key == TypesafeID<U>::NONE) {
        throw Exception("[In AssetRegistry::addToMap]: Key is NONE");
    }

    if (map.size() > static_cast<size_t>(key) && map.at(key) != nullptr) {
        throw DuplicateKeyError();
    }

    return *map.emplace(key, std::make_unique<T>(std::forward<Args>(args)...));
}

template<typename T, typename U>
auto trc::AssetRegistry::getFromMap(
    data::IndexMap<TypesafeID<U>, u_ptr<T>>& map,
    TypesafeID<U> key) -> T&
{
    if (key == TypesafeID<U>::NONE) {
        throw Exception("[In AssetRegistry::addToMap]: Key is NONE");
    }

    if (map.size() <= static_cast<size_t>(key) || map.at(key) == nullptr) {
        throw KeyNotFoundError();
    }

    return *map[key];
}
