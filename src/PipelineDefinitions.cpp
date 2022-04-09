#include "PipelineDefinitions.h"

#include <vkb/ShaderProgram.h>

#include "util/TorchDirectories.h"



auto trc::internal::loadShader(fs::path relPath) -> std::string
{
    if (relPath.is_absolute()) {
        relPath = relPath.string().substr(1);
    }

    return vkb::readFile(util::getInternalShaderStorageDirectory() / relPath);
}
