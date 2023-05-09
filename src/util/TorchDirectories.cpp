#include "trc/util/TorchDirectories.h"



namespace trc::util
{

auto getInternalShaderStorageDirectory() -> fs::path
{
    return TRC_SHADER_STORAGE_DIR;
}

auto getInternalShaderBinaryDirectory() -> fs::path
{
    return TRC_SHADER_BINARY_DIR;
}

} // namespace trc::util
