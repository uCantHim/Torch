#pragma once

#include "Boilerplate.h"
#include "Asset.h"

namespace trc
{
    struct Material : public Asset
    {
        vec4 colorAmbient;
        vec4 colorDiffuse;
        vec4 colorSpecular;

        float shininess{ 1.0f };

        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        ui32 diffuseTexture{ UINT32_MAX };
        ui32 specularTexture{ UINT32_MAX };
        ui32 bumpTexture{ UINT32_MAX };

        ui64 __padding;
    };
} // namespace trc
