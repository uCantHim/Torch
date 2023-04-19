#pragma once

#include "MaterialShaderProgram.h"
#include "ShaderCapabilityConfig.h"

namespace trc
{
    /**
     * @brief Create an object that defines all inputs that Torch provides to
     *        shaders
     *
     * @return ShaderCapabilityConfig A configuration for fragment shaders in
     *                                Torch's material system for standard
     *                                drawable objects.
     */
    auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig;

    /**
     * @brief Create an object that defines all possibly existing descriptors
     */
    auto makeShaderDescriptorConfig() -> ShaderDescriptorConfig;
} // namespace trc
