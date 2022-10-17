#pragma once

#include <string>

namespace trc
{
    enum class Capability
    {
        eVertexPosition,
        eVertexNormal,
        eVertexUV,

        eTime,
        eTimeDelta,

        eTextureSample,

        eNumCapabilities,
    };

    auto to_string(Capability capability) -> std::string;
} // namespace trc
