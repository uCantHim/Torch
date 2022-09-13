#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
namespace fs = std::filesystem;

#include <trc/Types.h>
#include <trc/assets/Assets.h>
using namespace trc::basic_types;

#include "AssetFileIndex.h"

/**
 * Provides access to a project's resource files.
 *
 * All writing operations will overwrite any file in the asset directory
 * that has not previously been registered as an asset file!
 */
class ProjectDirectory
{
public:
    ProjectDirectory(const ProjectDirectory&) = delete;
    ProjectDirectory(ProjectDirectory&&) noexcept = delete;
    ProjectDirectory& operator=(const ProjectDirectory&) = delete;
    ProjectDirectory& operator=(ProjectDirectory&&) noexcept = delete;

    /**
     * @brief Construct a project directory
     *
     * Tries to build an index from a `root/.assetIndex.json` file.
     * Constructs an empty index if this file is not found.
     *
     * @param const fs::path& root The filesystem directory
     *
     * @throw std::invalid_argument if `root` is not a directory
     */
    explicit ProjectDirectory(const fs::path& root);
    ~ProjectDirectory();

    /**
     * @brief Create a data source from a file
     *
     * @throw std::out_of_range if no asset exists at `path`.
     * @throw std::invalid_argument if an asset at `path` exists but is
     *        of incompatible type to `T`.
     */
    template<trc::AssetBaseType T>
    auto load(const trc::AssetPath& path) -> u_ptr<trc::AssetSource<T>>;

    /**
     * @brief Store an asset in the directory
     *
     * @param const trc::AssetPath& path
     * @param const trc::AssetData<T>& data
     * @param bool overwrite If true, this operation overwrites any
     *                       existing file with the given path.
     *
     * @return bool True if the operation has been successful. May be false
     *              if `overwrite` is false and the file already exists.
     */
    template<trc::AssetBaseType T>
    bool save(const trc::AssetPath& path, const trc::AssetData<T>& data, bool overwrite = true);

    /**
     * @brief Add an asset from an external file to the directory
     *
     * Copies data from an external file to a file in the asset directory.
     * Reads the contents of `srcPath` into memory to ensure that is has the
     * correct format.
     *
     * @param const fs::path& srcPath       Path from which to read data
     * @param const trc::AssetPath& dstPath Path at which to store the data
     */
    template<trc::AssetBaseType T>
    void add(const fs::path& srcPath, const trc::AssetPath& dstPath);

    /**
     * @return bool True if the path is a registered asset, false otherwise.
     */
    bool exists(const trc::AssetPath& path) const;

    /**
     * @brief Move an asset file to another location
     *
     * @throw std::invalid_argument if no asset exists at `from`.
     * @throw std::invalid_argument if an asset already exists at `to`.
     */
    void move(const trc::AssetPath& from, const trc::AssetPath& to);

    /**
     * @brief Remove a resource from the asset directory
     *
     * Warning: This deletes a file from disk!
     *
     * @throw std::invalid_argument if no asset exists at `path`.
     */
    void remove(const trc::AssetPath& path);

    /**
     * @brief Iterate over all known assets
     *
     * @param Visitor&& vis Resolves asset types.
     *        Must be a template `<typename T> void(const trc::AssetPath&)`.
     */
    template<typename Visitor>
    void foreach(Visitor&& vis);

private:
    const fs::path root;

    AssetFileIndex index;
    std::mutex fileWriteLock;
};

template<trc::AssetBaseType T>
auto ProjectDirectory::load(const trc::AssetPath& path) -> u_ptr<trc::AssetSource<T>>
{
    if (!index.contains(path)) {
        throw std::out_of_range(path.getUniquePath() + " is not a registered asset.");
    }
    if (index.getType(path) != toDynamicType<T>()) {
        throw std::invalid_argument("Unmatching type requested from " + path.getUniquePath());
    }

    return std::make_unique<trc::AssetPathSource<T>>(path);
}

template<trc::AssetBaseType T>
bool ProjectDirectory::save(
    const trc::AssetPath& path,
    const trc::AssetData<T>& data,
    bool overwrite)
{
    const auto fsPath = path.getFilesystemPath();
    if (!overwrite && fs::is_regular_file(fsPath)) {
        return false;
    }

    fs::create_directories(fsPath.parent_path());

    index.insert<T>(path);
    std::scoped_lock lock(fileWriteLock);
    std::ofstream file(fsPath, std::ios::binary);
    data.serialize(file);

    return true;
}

template<trc::AssetBaseType T>
void ProjectDirectory::add(const fs::path& srcPath, const trc::AssetPath& dstPath)
{
    // Load data and re-serialize it to make sure the src format is valid.
    saveToFile(loadAssetData<T>(srcPath), dstPath);
    index.insert<T>(dstPath);
}

template<typename Visitor>
void ProjectDirectory::foreach(Visitor&& vis)
{
    index.foreach(std::forward<Visitor>(vis));
}
