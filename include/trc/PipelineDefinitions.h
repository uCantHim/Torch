#pragma once

#include <string>
#include <filesystem>

#include "trc/ShaderPath.h"

namespace trc::internal
{
    namespace fs = std::filesystem;

    auto loadShader(const ShaderPath& path) -> std::string;
} // namespace trc::internal
