#pragma once

#include "Boilerplate.h"
#include "utils/Util.h"

namespace trc
{
    constexpr ui32 NO_TEXTURE = UINT32_MAX;

    struct Material
    {
        vec4 colorAmbient{ 1.0f, 0.0f, 1.0f, 1.0f };
        vec4 colorDiffuse{ 1.0f, 0.0f, 1.0f, 1.0f };
        vec4 colorSpecular;

        float shininess{ 1.0f };

        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        ui32 diffuseTexture{ NO_TEXTURE };
        ui32 specularTexture{ NO_TEXTURE };
        ui32 bumpTexture{ NO_TEXTURE };

        ui64 __padding;
    };

    static_assert(util::sizeof_pad_16_v<Material> == sizeof(Material),
                  "The Material struct must always be padded to 16 bytes");
} // namespace trc
