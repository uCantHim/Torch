#include "AssetManager.h"



trc::AssetManager::AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo)
    :
    registry(instance, arInfo)
{
}

auto trc::AssetManager::getDeviceRegistry() -> AssetRegistry&
{
    return registry;
}
