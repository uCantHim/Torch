#include "trc/material/RuntimeResourceHandler.h"

#include "trc/material/MaterialRuntime.h"



namespace trc
{

RuntimePushConstantHandler::RuntimePushConstantHandler(const ShaderResources& resources)
{
    constexpr ui32 alloc = std::numeric_limits<ui32>::max();

    for (auto [offset, size, userId] : resources.getPushConstants())
    {
        offsets.resize(glm::max(size_t{userId + 1}, offsets.size()), alloc);
        offsets[userId] = offset;
    }
}

} // namespace trc
