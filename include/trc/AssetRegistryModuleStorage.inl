#include "AssetRegistryModuleStorage.h"



namespace trc
{

template<AssetRegistryModuleType T, typename ...Args>
    requires std::constructible_from<T, Args...>
void AssetRegistryModuleStorage::addModule(Args&&... args)
{
    entries.emplace(StaticIndex<T>::index, std::make_unique<T>(std::forward<Args>(args)...));
}

template<AssetRegistryModuleType T>
auto AssetRegistryModuleStorage::get() -> T&
{
    assert(entries.size() > StaticIndex<T>::index);
    assert(entries.get(StaticIndex<T>::index).valid());

    return entries.get(StaticIndex<T>::index).template as<T>();
}

template<AssetRegistryModuleType T>
auto AssetRegistryModuleStorage::get() const -> const T&
{
    assert(entries.size() > StaticIndex<T>::index);
    assert(entries.get(StaticIndex<T>::index).valid());

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
