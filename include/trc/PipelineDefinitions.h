#pragma once

#include <string>
#include <filesystem>

#include "trc/ShaderPath.h"

namespace trc
{
    class ShaderLoader;
} // namespace trc

namespace trc::internal
{
    auto getShaderLoader() -> const ShaderLoader&;
    auto loadShader(const ShaderPath& path) -> std::string;
} // namespace trc::internal
