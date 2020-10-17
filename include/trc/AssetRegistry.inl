


template<typename T, typename U, typename... Args>
auto trc::AssetRegistry::addToMap(
    data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
    TypesafeID<U> key,
    Args&&... args) -> T&
{
    assert(static_cast<ui32>(key) != UINT32_MAX);  // Reserved ID that signals empty value

    if (map.size() > static_cast<size_t>(key) && map.at(key) != nullptr) {
        throw DuplicateKeyError();
    }

    return *map.emplace(key, std::make_unique<T>(std::forward<Args>(args)...));
}

template<typename T, typename U>
auto trc::AssetRegistry::getFromMap(
    data::IndexMap<TypesafeID<U>, std::unique_ptr<T>>& map,
    TypesafeID<U> key) -> T&
{
    assert(static_cast<ui32>(key) != UINT32_MAX);  // Reserved ID that signals empty value

    if (map.size() <= static_cast<size_t>(key) || map.at(key) == nullptr) {
        throw KeyNotFoundError();
    }

    return *map[key];
}



template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addGeometry(const NameType& key, Geometry geo)
    -> Geometry&
{
    auto result = AssetRegistry::addGeometry(std::move(geo));

    auto [it, success] = geometryNames.emplace(key, result);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return *AssetRegistry::getGeometry(result).get();
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addMaterial(const NameType& key, Material mat)
    -> Material&
{
    auto result = AssetRegistry::addMaterial(std::move(mat));

    auto [it, success] = materialNames.emplace(key, result);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return *AssetRegistry::getMaterial(result).get();
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addImage(const NameType& key, vkb::Image img)
    -> vkb::Image&
{
    auto result = AssetRegistry::addImage(std::move(img));

    auto [it, success] = imageNames.emplace(key, result);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return *AssetRegistry::getImage(result).get();
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometry(const NameType& key) -> Maybe<Geometry*>
{
    return getGeometryIndex(key).maybe(
        [](GeometryID index) { return AssetRegistry::getGeometry(index); },
        []()                 { return Maybe<Geometry*>(); }
    );
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getMaterial(const NameType& key) -> Maybe<Material*>
{
    return getMaterialIndex(key).maybe(
        [](MaterialID index) { return AssetRegistry::getMaterial(index); },
        []()                 { return Maybe<Material*>(); }
    );
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getImage(const NameType& key) -> Maybe<vkb::Image*>
{
    return getImageIndex(key).maybe(
        [](TextureID index) { return AssetRegistry::getImage(index); },
        []()                { return Maybe<vkb::Image*>(); }
    );
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometryIndex(const NameType& key)
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
auto trc::AssetRegistryNameWrapper<NameType>::getMaterialIndex(const NameType& key)
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
auto trc::AssetRegistryNameWrapper<NameType>::getImageIndex(const NameType& key)
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
