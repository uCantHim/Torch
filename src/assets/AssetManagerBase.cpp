#include "trc/assets/AssetManagerBase.h"



namespace trc
{

InvalidAssetIdError::InvalidAssetIdError(ui32 id, std::string_view reason)
    :
    Exception(
        "Asset ID " + std::to_string(id) + " is invalid"
        + (reason.empty() ? "." : (": " + std::string(reason)))
    )
{}

auto AssetManagerBase::getMetadata(AssetID id) const -> const AssetMetadata&
{
    if (!assetInformation.contains(ui32{id})) {
        throw InvalidAssetIdError(ui32{id}, "No asset with this ID exists in the asset manager"
                                            " - has the asset already been destroyed?");
    }

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
