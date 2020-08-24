


template<typename T>
auto trc::AssetRegistry::addToMap(
    data::IndexMap<ui32, std::unique_ptr<T>>& map,
    ui32 key, T value) -> T&
{
    static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>, "");
    assert(key != UINT32_MAX);  // Reserved ID that signals empty value

    if (map.size() > key && map[key] != nullptr) {
        throw DuplicateKeyError();
    }

    return *map.emplace(key, std::make_unique<T>(std::move(value)));
}

template<typename T>
auto trc::AssetRegistry::getFromMap(data::IndexMap<ui32, std::unique_ptr<T>>& map, ui32 key) -> T&
{
    assert(key != UINT32_MAX);  // Reserved ID that signals empty value

    if (map[key] == nullptr) {
        throw KeyNotFoundError();
    }

    return *map[key];
}



template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addGeometry(const NameType& key, Geometry geo)
    -> std::pair<Ref<Geometry>, ui32>
{
    auto result = AssetRegistry::addGeometry(std::move(geo));

    auto [it, success] = geometryNames.emplace(key, result.second);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return result;
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addMaterial(const NameType& key, Material mat)
    -> std::pair<Ref<Material>, ui32>
{
    auto result = AssetRegistry::addMaterial(std::move(mat));

    auto [it, success] = materialNames.emplace(key, result.second);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return result;
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::addImage(const NameType& key, vkb::Image img)
    -> std::pair<Ref<vkb::Image>, ui32>
{
    auto result = AssetRegistry::addImage(std::move(img));

    auto [it, success] = imageNames.emplace(key, result.second);
    if (!success) {
        throw trc::DuplicateKeyError();
    }

    return result;
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometry(const NameType& key) -> Geometry&
{
    try {
        return AssetRegistry::getGeometry(geometryNames[key]);
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getMaterial(const NameType& key) -> Material&
{
    try {
        return AssetRegistry::getMaterial(materialNames[key]);
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getImage(const NameType& key) -> vkb::Image&
{
    try {
        return AssetRegistry::getImage(imageNames[key]);
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getGeometryIndex(const NameType& key) -> ui32
{
    try {
        return geometryNames[key];
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getMaterialIndex(const NameType& key) -> ui32
{
    try {
        return materialNames[key];
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}

template<typename NameType>
auto trc::AssetRegistryNameWrapper<NameType>::getImageIndex(const NameType& key) -> ui32
{
    try {
        return imageNames[key];
    }
    catch (const std::out_of_range&) {
        throw KeyNotFoundError();
    }
}
