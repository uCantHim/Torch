#include "AssetRegistryModuleStorage.h"

#include <iostream>



namespace trc
{

template<AssetRegistryModuleType T>
void AssetRegistryModuleStorage::addModule()
{
    entries[StaticIndex<T>::index] = TypeEntry(std::make_unique<T>(createInfo));
}

template<AssetRegistryModuleType T>
auto AssetRegistryModuleStorage::get() -> T&
{
    if (!entries[StaticIndex<T>::index].valid())
    {
        addModule<T>();
    }

    return entries.at(StaticIndex<T>::index).template as<T>();
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
