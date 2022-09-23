#include "PipelineDefinitions.h"

#include <vkb/ShaderProgram.h>

#include "util/TorchDirectories.h"
#include "ShaderLoader.h"
#include "ShaderPath.h"



auto trc::internal::loadShader(const ShaderPath& path) -> std::string
{
    static ShaderLoader loader(
        {
            util::getInternalShaderBinaryDirectory(),
            util::getInternalShaderStorageDirectory()
        },
        // Binary output directory:
        util::getInternalShaderBinaryDirectory()
    );

    return loader.load(path);
}
