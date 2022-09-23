#include "trc/ShaderPath.h"



namespace trc
{

ShaderPath::ShaderPath(fs::path path)
    :
    pathlet(std::move(path))
{
}

auto ShaderPath::getSourcePath() const -> const util::Pathlet&
{
    return pathlet;
}

auto ShaderPath::getBinaryPath() const -> util::Pathlet
{
    return pathlet.withExtension(".spv");
}

} // namespace trc
