#include "trc/util/Pathlet.h"



namespace trc::util
{

auto makeRelativePathlet(fs::path path) -> fs::path
{
    if (path.empty()) {
        throw std::invalid_argument("Unable to construct unique asset path from \""
                                    + path.string() + "\": " + "Path is empty.");
    }

    if (path == "."
        || path.string().ends_with("/")
        || path.string().ends_with(fs::path::preferred_separator))
    {
        throw std::invalid_argument("Unable to construct unique asset path from \""
                                    + path.string() + "\": " + "Path must not be a directory name.");
    }

    // Replace any root directory names
    if (path.has_root_path()) {
        path = path.string().substr(path.root_path().string().size());
    }

    return path.lexically_normal();
}



Pathlet::Pathlet(fs::path path)
    :
    pathlet(makeRelativePathlet(std::move(path)))
{
}

auto Pathlet::string() const -> std::string
{
    return pathlet.string();
}

auto Pathlet::filename() const -> fs::path
{
    return pathlet.filename();
}

auto Pathlet::replaceExtension(const std::string& newExt) const -> Pathlet
{
    return Pathlet(fs::path{ pathlet }.replace_extension(newExt));
}

auto Pathlet::withExtension(const std::string& ext) const -> Pathlet
{
    return Pathlet(string() + ext);
}

auto Pathlet::filesystemPath(const fs::path& parentPath) const -> fs::path
{
    return parentPath / pathlet;
}

auto operator/(const fs::path& parentPath, const Pathlet& pathlet) -> fs::path
{
    return pathlet.filesystemPath(parentPath);
}

} // namespace trc::util
