#include "AssetRegistryModuleStorage.h"



namespace trc
{

AssetRegistryModuleStorage::AssetRegistryModuleStorage(const AssetRegistryModuleCreateInfo& info)
{
    for (uint32_t i = 0; const auto& factory : types.factories)
    {
        entries.emplace(i, factory(info));
    }
}

void AssetRegistryModuleStorage::foreach(std::function<void(AssetRegistryModuleInterface&)> func)
{
    for (auto& entry : entries)
    {
        if (entry.valid())
        {
            func(entry.as<AssetRegistryModuleInterface>());
        }
    }
}



AssetRegistryModuleStorage::TypeEntry::TypeEntry(TypeEntry&& other) noexcept
    :
    ptr(other.ptr),
    _delete(other._delete)
{
    other.ptr = nullptr;
    other._delete = nullptr;
}

auto AssetRegistryModuleStorage::TypeEntry::operator=(TypeEntry&& other) noexcept -> TypeEntry&
{
    if (this != &other)
    {
        std::swap(ptr, other.ptr);
        std::swap(_delete, other._delete);
    }
    return *this;
}

AssetRegistryModuleStorage::TypeEntry::~TypeEntry()
{
    if (ptr != nullptr)
    {
        assert(_delete != nullptr);
        _delete(ptr);
    }
}

bool AssetRegistryModuleStorage::TypeEntry::valid() const
{
    return ptr != nullptr;
}

} // namespace trc
