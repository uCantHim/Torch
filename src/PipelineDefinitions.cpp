#include "trc/PipelineDefinitions.h"

#include "trc/base/ShaderProgram.h"

#include "trc/util/TorchDirectories.h"
#include "trc/ShaderLoader.h"
#include "trc/ShaderPath.h"



auto trc::internal::loadShader(const ShaderPath& path) -> std::string
{
    static ShaderLoader loader(
        {
            util::getInternalShaderStorageDirectory(),
            util::getInternalShaderBinaryDirectory()
        },
        // Binary output directory:
        util::getInternalShaderBinaryDirectory()
    );

    return loader.load(path);
}
