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
     * @param fs::path path Must be a path to a directory.
     *
     * @throw std::invalid_argument if `path` is not a valid path to a
     *        directory.
     */
    void setProjectDirectory(fs::path newPath);

    /**
     * @return fs::path A directory where Torch saves its assets.
     */
    auto getAssetStorageDirectory() -> fs::path;

    /**
     * @return fs::path Path to directory in which user shaders are stored.
     */
    auto getShaderStorageDirectory() -> fs::path;

    /**
     * @return fs::path Path to directory in which Torch's shaders are
     *         stored.
     */
    auto getInternalShaderStorageDirectory() -> fs::path;
} // namespace trc
