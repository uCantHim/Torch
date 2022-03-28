#include "AssetSource.h"

#include "assets/AssetDataProxy.h"



auto trc::AssetSourceLoadHelper::loadAsset(const AssetPath& path) -> TypeProxy
{
    TypeProxy result;
    AssetDataProxy data(path);
    data.visit([&result](auto dataType) { result.data = std::move(dataType); });

    return result;
}
