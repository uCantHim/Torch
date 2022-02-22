#include "AssetRegistryModuleStorage.h"



namespace trc
{

template<AssetRegistryModuleType T, typename ...Args>
    requires std::constructible_from<T, Args...>
          && std::derived_from<T, AssetRegistryModuleInterface>
void AssetRegistryModuleStorage::addModule(Args&&... args)
{
    std::scoped_lock lock(entriesLock);
    entries.emplace(StaticIndex<T>::index, std::make_unique<T>(std::forward<Args>(args)...));
}

template<AssetRegistryModuleType T>
auto AssetRegistryModuleStorage::get() -> T&
{
    std::scoped_lock lock(entriesLock);
    const bool hasModule = entries.has(StaticIndex<T>::index)
                        && entries.get(StaticIndex<T>::index).valid();
    if (!hasModule)
    {
        throw std::invalid_argument(
            "[In AssetRegistryModuleStorage::get]: Requested asset registry module type (static"
            " index " + std::to_string(StaticIndex<T>::index) + ") does not exist in the module"
            " storage!"
        );
    }

    return entries.get(StaticIndex<T>::index).template as<T>();
}



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

} // namespace trc
