#pragma once

#include <vector>

#include "trc/ShaderPath.h"
#include "trc/Types.h"

namespace trc
{
    class ShaderLoader;
} // namespace trc

namespace trc::internal
{
    auto getShaderLoader() -> const ShaderLoader&;
    auto loadShader(const ShaderPath& path) -> std::vector<ui32>;
} // namespace trc::internal
