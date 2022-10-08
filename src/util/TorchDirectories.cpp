#include "trc/util/TorchDirectories.h"



namespace trc::util
{

constexpr const char* ASSET_STORAGE_DIR_NAME{ "assets" };
constexpr const char* SHADER_STORAGE_DIR_NAME{ "shaders" };

fs::path projectDirectory{ TRC_COMPILE_ROOT_DIR"/project_files" };

auto getProjectDirectory() -> fs::path
{
    return projectDirectory;
}

void setProjectDirectory(fs::path newPath)
{
    projectDirectory = std::move(newPath);
}

auto getAssetStorageDirectory() -> fs::path
{
    return getProjectDirectory() / ASSET_STORAGE_DIR_NAME;
}

auto getShaderStorageDirectory() -> fs::path
{
    return getProjectDirectory() / SHADER_STORAGE_DIR_NAME;
}

auto getInternalShaderStorageDirectory() -> fs::path
{
    return TRC_COMPILE_ROOT_DIR"/shaders";
}

auto getInternalShaderBinaryDirectory() -> fs::path
{
    return TRC_SHADER_BINARY_DIR;
}

} // namespace trc::util
