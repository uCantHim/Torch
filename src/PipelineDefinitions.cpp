#include "PipelineDefinitions.h"

#include <vkb/ShaderProgram.h>

#include "util/TorchDirectories.h"
#include "ShaderPath.h"



auto trc::internal::loadShader(fs::path relPath) -> std::string
{
    trc::ShaderPath path(relPath);

    return vkb::readFile(util::getInternalShaderStorageDirectory() / path.getBinaryName());
}
