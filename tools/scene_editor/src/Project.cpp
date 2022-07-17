#include "Project.h"

#include <trc/util/TorchDirectories.h>

#include "App.h"



namespace g
{

auto project() -> Project&
{
    return App::get().getProject();
}

auto assetStore() -> ProjectDirectory&
{
    return project().getStorageDir();
}

} // namespace g

Project::Project(const fs::path& rootDir)
    :
    rootDir(rootDir),
    storageDir(std::make_unique<ProjectDirectory>(rootDir))
{
}

auto Project::getRootDirectory() const -> const fs::path&
{
    return rootDir;
}

auto Project::getStorageDir() -> ProjectDirectory&
{
    return *storageDir;
}
