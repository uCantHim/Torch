#include "trc/material/RuntimeResourceHandler.h"

#include "trc/material/MaterialRuntime.h"



namespace trc
{

RuntimePushConstantHandler::RuntimePushConstantHandler(
    const ShaderResources& resources,
    const MaterialRuntimeConfig& conf)
{
    constexpr ui32 alloc = std::numeric_limits<ui32>::max();

    for (auto [pc, resource] : conf.pushConstantIds)
    {
        offsets.resize(glm::max(size_t{pc + 1}, offsets.size()), alloc);
        if (auto info = resources.getPushConstantInfo(resource)) {
            offsets[pc] = info->offset;
        }
    }
}

} // namespace trc
