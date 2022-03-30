#pragma once

#include <cassert>
#include <string>
#include <filesystem>

#include <trc_util/Exception.h>

namespace trc
{
    namespace fs = std::filesystem;

    class InvalidAssetPathError : public Exception
    {
    public:
        InvalidAssetPathError(fs::path pathlet, std::string error);
    };

    class AssetPath
    {
    public:
        /**
         * @throw InvalidAssetPathError if no valid AssetPath can be constructed.
         */
        AssetPath(fs::path path);

        auto getUniquePath() const -> std::string;
        auto getFilesystemPath() const -> fs::path;
        auto getAssetName() const -> std::string;

        auto operator<=>(const AssetPath&) const = default;

    private:
        /** Path relative to the asset directory */
        fs::path pathlet;
    };
} // namespace trc


namespace std
{
    template<>
    struct hash<::trc::AssetPath>
    {
        auto operator()(const ::trc::AssetPath& path) const noexcept
        {
            return hash<std::string>{}(path.getFilesystemPath());
        }
    };
}
