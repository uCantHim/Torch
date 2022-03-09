#include "AssetRegistryModuleStorage.h"



namespace trc
{

template<typename T>
auto AssetRegistryModuleStorage::get() -> T&
{
    return entries.at(StaticIndex<T>::index).template as<T>();
}

template<typename T>
auto AssetRegistryModuleStorage::get() const -> const T&
{
    return entries.at(StaticIndex<T>::index).template as<T>();
}

template<typename T>
const bool AssetRegistryModuleStorage::StaticIndex<T>::_init{
    AssetRegistryModuleStorage::types.template add<T>()
};

template<typename T>
AssetRegistryModuleStorage::TypeEntry::TypeEntry(std::unique_ptr<T> obj)
    :
    ptr(obj.release()),
    _delete([](void* ptr) { delete static_cast<T*>(ptr); })
{
}

template<typename T>
auto AssetRegistryModuleStorage::TypeEntry::as() -> T&
{
    assert(ptr != nullptr);
    return *static_cast<T*>(ptr);
}

template<typename T>
auto AssetRegistryModuleStorage::TypeEntry::as() const -> const T&
{
    assert(ptr != nullptr);
    return *static_cast<const T*>(ptr);
}

template<typename T>
    requires std::is_constructible_v<T, const AssetRegistryModuleCreateInfo&>
void AssetRegistryModuleStorage::TypeRegistrations::add()
{
    factories.emplace(
        StaticIndex<T>::index,
        [](const AssetRegistryModuleCreateInfo& info)
        {
            return TypeEntry(std::make_unique<T>(info));
        }
    );
}

} // namespace trc
