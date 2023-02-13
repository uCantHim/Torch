#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>

namespace trc::util
{
    namespace fs = std::filesystem;

    /**
     * For typesafety, use the Pathlet class. Its constructor invokes this
     * function and wraps the result in a typesafe interface.
     *
     * @throw std::invalid_argument if the given path does not satisfy the
     *        consistency requirements.
     */
    auto makeRelativePathlet(fs::path path) -> fs::path;

    /**
     * @brief A path-fragment relative to a root path
     *
     * A short(er) relative path for use in contexts where full file paths
     * are implicit or unknown before being used to access physical files.
     *
     * In other words, Pathlets allow directories to be logical until they
     * are really needed.
     *
     * For example, in the context of AssetPaths, a path to an asset may be
     * specified as a value `Pathlet("foo/bar/image.png")`. This format is
     * useful because it does not depend on installation-dependent absolute
     * file paths. Additionally, its shortness is much more user friendly
     * than full file paths. If we now want to load the image from disk, we
     * can find the actual location of the project's asset directory only
     * when needed: `"/home/alice/assets" / pathlet`. This would yield the
     * absolute path `/home/alice/assets/foo/bar/image.png`.
     */
    class Pathlet
    {
    public:
        Pathlet(const Pathlet&) = default;
        Pathlet(Pathlet&&) noexcept = default;
        Pathlet& operator=(const Pathlet&) = default;
        Pathlet& operator=(Pathlet&&) noexcept = default;
        ~Pathlet() noexcept = default;

        /**
         * Empty pathlets are not allowed.
         *
         * @throw std::invalid_argument if the given path does not satisfy the
         *        consistency requirements.
         */
        explicit Pathlet(fs::path path);

        /**
         * @return std::string The pathlet as a plain string
         */
        auto string() const -> std::string;

        /**
         * @return fs::path The file's name stripped of leading directories
         */
        auto filename() const -> fs::path;

        /**
         * @brief Replace the outer-most extension with a string
         */
        auto replaceExtension(const std::string& newExt) const -> Pathlet;

        /**
         * @brief Append an extension to the pathlet
         */
        auto withExtension(const std::string& ext) const -> Pathlet;

        /**
         * @return fs::path The result of the path-concatenation
         *         `parentPath / pathlet`.
         */
        auto filesystemPath(const fs::path& parentPath) const -> fs::path;

        auto operator<=>(const Pathlet&) const = default;

    private:
        fs::path pathlet;
    };

    /**
     * @brief Concatenate a filesystem path and a pathlet
     *
     * The Pathlet can only occur at the operator's left-hand-side.
     *
     * @param fs::path parentPath The parent path.
     * @param Pathlet  pathlet    The pathlet relative to the given parent
     *                            path.
     */
    auto operator/(const fs::path& parentPath, const Pathlet& pathlet) -> fs::path;
} // namespace trc

/**
 * @brief std::hash specialization for Pathlet
 */
template<>
struct std::hash<trc::util::Pathlet>
{
    auto operator()(const trc::util::Pathlet& path) const noexcept
    {
        return hash<std::string>{}(path.string());
    }
};
