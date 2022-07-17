#pragma once

#include "ProjectDirectory.h"

class Project;

namespace g
{
    auto project() -> Project&;
    auto assetStore() -> ProjectDirectory&;
}

class Project
{
public:
    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;

    Project(Project&&) noexcept = default;
    Project& operator=(Project&&) noexcept = default;
    ~Project() = default;

    explicit Project(const fs::path& rootDir);

    auto getRootDirectory() const -> const fs::path&;

    auto getStorageDir() -> ProjectDirectory&;

private:
    fs::path rootDir;
    u_ptr<ProjectDirectory> storageDir;
};
