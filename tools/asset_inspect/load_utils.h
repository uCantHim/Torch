#pragma once

#include <expected>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace fs = std::filesystem;

#include <trc/assets/AssetBase.h>

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
        exit(1);
    }

    // Try to parse geometry from file
    try {
        trc::AssetData<T> data;
        data.deserialize(file);
        return data;
    }
    catch (const std::exception& err) {
        return std::unexpected("Unable to parse geometry file " + path.string()
                               + ": " + err.what());
        exit(1);
    }
}
