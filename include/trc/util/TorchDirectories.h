#pragma once

#include <filesystem>

namespace trc::util
{
    namespace fs = std::filesystem;

    /**
     * I don't have a project mechanism yet, so this is just a folder in
     * Torch's root directory; One Torch download is one project.
     */
    auto getProjectDirectory() -> fs::path;

    /**
     * @return fs::path A directory where Torch saves its assets.
     */
    auto getAssetStorageDirectory() -> fs::path;
} // namespace trc
