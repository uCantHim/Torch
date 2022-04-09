#include "util/TorchDirectories.h"

#include <iostream>



namespace trc::util
{

constexpr const char* ASSET_STORAGE_DIR_NAME{ "assets" };
constexpr const char* SHADER_STORAGE_DIR_NAME{ "shaders" };

fs::path projectDirectory{ TRC_COMPILE_ROOT_DIR"/project_files" };

auto getProjectDirectory() -> fs::path
{
    return projectDirectory;
}

auto setProjectDirectory(fs::path newPath)
{
    projectDirectory = std::move(newPath);

#ifdef TRC_DEBUG
    static bool firstExecution{ true };
    if (!firstExecution)
    {
        std::cout << "-- Warning: Project directory has been set more than once. This is not "
            << "recommended!\n";
    }
    firstExecution = false;
#endif
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
    return TRC_SHADER_DIR;
}

}
