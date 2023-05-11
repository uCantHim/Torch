#pragma once

#include <trc/assets/AssetStorage.h>

using namespace trc::basic_types;

class Project;

namespace g
{
    auto project() -> Project&;
}

class Project
{
public:
    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;
    Project& operator=(Project&&) noexcept = delete;

    Project(Project&&) noexcept = default;
    ~Project() = default;

    explicit Project(trc::AssetStorage& assetStorage);

    auto getStorageDir() -> trc::AssetStorage&;

private:
    trc::AssetStorage& assetStorage;
};
