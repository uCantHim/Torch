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
    assertExists(id, "has the asset already been destroyed?");
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

void AssetManagerBase::assertExists(AssetID id, std::string_view hint) const
{
    if (!assetInformation.contains(ui32{id})) {
        throw InvalidAssetIdError(ui32{id}, "No asset with this ID exists in the asset manager"
                                            " - " + std::string(hint));
    }
}

auto AssetManagerBase::begin() const -> const_iterator
{
    return assetInformation.begin();
}

auto AssetManagerBase::end() const -> const_iterator
{
    return assetInformation.end();
}

} // namespace trc
