


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

    return AssetRegistry::getGeometry(result);
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

    return AssetRegistry::getMaterial(result);
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

    return AssetRegistry::getImage(result);
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometry(const NameType& key) -> Geometry&
{
    return AssetRegistry::getGeometry(getGeometryIndex(key));
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getMaterial(const NameType& key) -> Material&
{
    return AssetRegistry::getMaterial(getMaterialIndex(key));
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getImage(const NameType& key) -> vkb::Image&
{
    return AssetRegistry::getImage(getImageIndex(key));
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometryIndex(const NameType& key) -> ui32
{
    try {
        return geometryNames.at(key);
    }
    catch (const std::out_of_range&) {
        return 0;
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getMaterialIndex(const NameType& key) -> ui32
{
    try {
        return materialNames.at(key);
    }
    catch (const std::out_of_range&) {
        return 0;
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getImageIndex(const NameType& key) -> ui32
{
    try {
        return imageNames.at(key);
    }
    catch (const std::out_of_range&) {
        return 0;
    }
}
