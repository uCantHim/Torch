#pragma once

#include "MaterialShaderProgram.h"
#include "ShaderCapabilityConfig.h"

namespace trc
{
    auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig;
    auto makeShaderDescriptorConfig() -> ShaderDescriptorConfig;
} // namespace trc
