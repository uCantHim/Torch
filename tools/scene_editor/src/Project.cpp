#include "Project.h"

#include "App.h"



namespace g
{

auto project() -> Project&
{
    return App::get().getProject();
}

} // namespace g

Project::Project(trc::AssetStorage& assetStorage)
    :
    assetStorage(assetStorage)
{
}

auto Project::getStorageDir() -> trc::AssetStorage&
{
    return assetStorage;
}
