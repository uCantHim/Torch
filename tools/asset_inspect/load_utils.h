#pragma once

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
namespace fs = std::filesystem;

#include <trc/assets/AssetBase.h>
#include <trc/assets/Serializer.h>

template<trc::AssetBaseType T>
inline auto tryLoad(const fs::path& path) -> std::expected<trc::AssetData<T>, std::string>
{
    // Try to open file
    if (!fs::is_regular_file(path)) {
        return std::unexpected(path.string() + " is not a regular file");
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected("Unable to open file " + path.string());
    }

    // Try to parse asset from file
    auto data = trc::AssetSerializerTraits<T>::deserialize(file);
    if (data) {
        return data.value();
    }

    return std::unexpected("Unable to parse an asset of type \"" + std::string{T::name()}
                           + "\" from file " + path.string() + ": " + data.error().message);
}
