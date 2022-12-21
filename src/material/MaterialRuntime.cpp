#include "trc/material/MaterialRuntime.h"



namespace trc
{

MaterialRuntime::MaterialRuntime(
    Pipeline::ID pipeline,
    std::vector<ShaderResources::PushConstantInfo> pushConstantConfig,
    std::vector<TextureHandle> loadedTextures)
    :
    pipeline(pipeline),
    loadedTextures(std::move(loadedTextures))
{
    constexpr ui32 alloc = std::numeric_limits<ui32>::max();

    for (auto [offset, size, userId] : pushConstantConfig)
    {
        pcOffsets.resize(glm::max(size_t{userId + 1}, pcOffsets.size()), alloc);
        pcOffsets[userId] = offset;
    }
}

auto MaterialRuntime::getPipeline() const -> Pipeline::ID
{
    return pipeline;
}

} // namespace trc
