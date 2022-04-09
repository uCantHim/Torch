#pragma once

#include <string>
#include <filesystem>

namespace trc::internal
{
    namespace fs = std::filesystem;

    auto loadShader(fs::path relPath) -> std::string;
} // namespace trc::internal
