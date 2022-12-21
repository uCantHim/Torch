#pragma once

#include "trc/Types.h"
#include "trc/assets/AssetManager.h"
#include "trc/assets/AssetReference.h"
#include "trc/assets/MaterialRegistry.h"
#include "trc/assets/Texture.h"

namespace trc
{
    /**
     * Used for a simplified material creation process as well as for
     * material imports.
     */
    struct SimpleMaterialData
    {
        vec3 color{ 0.0f, 0.0f, 0.0f };

        float specularCoefficient{ 1.0f };
        float roughness{ 1.0f };
        float metallicness{ 0.0f };

        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        bool doPerformLighting{ true };

        AssetReference<Texture> albedoTexture{};
        AssetReference<Texture> normalTexture{};
    };
} // namespace trc
