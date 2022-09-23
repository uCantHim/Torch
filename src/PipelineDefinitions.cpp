#include "PipelineDefinitions.h"

#include <vkb/ShaderProgram.h>

#include "util/TorchDirectories.h"
#include "ShaderPath.h"



auto trc::internal::loadShader(const ShaderPath& path) -> std::string
{
    return vkb::readFile(util::getInternalShaderStorageDirectory() / path.getBinaryName());
}
