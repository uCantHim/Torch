#include "util/TorchDirectories.h"


namespace trc::util
{

constexpr const char* ASSET_STORAGE_DIR_NAME{ "assets" };

auto getProjectDirectory() -> fs::path
{
    return TRC_PROJECT_DIR;
}

auto getAssetStorageDirectory() -> fs::path
{
    return getProjectDirectory() / ASSET_STORAGE_DIR_NAME;
}

}
