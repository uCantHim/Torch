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



// ----------------------- //
//      Named Wrapper      //
// ----------------------- //

template<typename NameType>
trc::AssetRegistryNamedWrapper<NameType>::AssetRegistryNamedWrapper(AssetRegistry& ar)
    : ar(&ar)
{
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::add(const NameType& key, GeometryData geo)
    -> GeometryID
{
    auto id = ar->add(std::move(geo));

    auto [it, success] = geometryNames.emplace(key, id);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return id;
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::add(const NameType& key, Material mat)
    -> MaterialID
{
    auto id = ar->add(std::move(mat));

    auto [it, success] = materialNames.emplace(key, id);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return id;
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::add(const NameType& key, vkb::Image img)
    -> TextureID
{
    auto id = ar->add(std::move(img));

    auto [it, success] = imageNames.emplace(key, id);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return id;
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getGeo(const NameType& key) -> Maybe<Geometry>
{
    return getGeometryIndex(key)
        >> [this](GeometryID index) { return ar->get(index); };
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getMat(const NameType& key) -> Maybe<Material&>
{
    return getMaterialIndex(key)
        >> [this](MaterialID index) { return ar->get(index); };
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getTex(const NameType& key) -> Maybe<Texture>
{
    return getImageIndex(key)
        >> [this](TextureID index) { return ar->get(index); };
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getGeoIndex(const NameType& key)
    -> Maybe<GeometryID>
{
    auto it = geometryNames.find(key);
    if (it != geometryNames.end()) {
        return it->second;
    }
    else {
        return {};
    }
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getMatIndex(const NameType& key)
    -> Maybe<MaterialID>
{
    auto it = materialNames.find(key);
    if (it != materialNames.end()) {
        return it->second;
    }
    else {
        return {};
    }
}

template<typename NameType>
auto trc::AssetRegistryNamedWrapper<NameType>::getTexIndex(const NameType& key)
    -> Maybe<TextureID>
{
    auto it = imageNames.find(key);
    if (it != imageNames.end()) {
        return it->second;
    }
    else {
        return {};
    }
}
