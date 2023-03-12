#include "trc/assets/AssetManagerBase.h"



namespace trc
{

AssetManagerBase::AssetManagerBase(
    const Instance& instance,
    const AssetRegistryCreateInfo& deviceRegistryCreateInfo)
    :
    deviceRegistry(instance, deviceRegistryCreateInfo)
{
}

auto AssetManagerBase::getMetadata(AssetID id) const -> const AssetMetadata&
{
    assert(assetInformation.contains(ui32{id}));
    return assetInformation.at(ui32{id}).getMetadata();
}

auto AssetManagerBase::getAssetType(AssetID id) const -> const AssetType&
{
    return getMetadata(id).type;
}

auto AssetManagerBase::getDeviceRegistry() -> AssetRegistry&
{
    return deviceRegistry;
}

} // namespace trc
