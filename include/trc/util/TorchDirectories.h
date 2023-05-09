#pragma once

#include <filesystem>

namespace trc::util
{
    namespace fs = std::filesystem;

    /**
     * @return fs::path Path to directory in which Torch's shader sources
     *         reside.
     */
    auto getInternalShaderStorageDirectory() -> fs::path;

    /**
     * @return fs::path Path to directory in which Torch's shader binaries
     *         are stored.
     */
    auto getInternalShaderBinaryDirectory() -> fs::path;
} // namespace trc
