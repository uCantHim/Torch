#pragma once

#include "Types.h"
#include "trc_util/Padding.h"

namespace trc
{
    constexpr ui32 NO_TEXTURE = UINT32_MAX;

    struct Material
    {
        /**
         * @brief A plain color to be used if no diffuse texture is present
         *
         * The alpha value is used as the material's opacity.
         */
        vec4 color{ 0.0f, 0.0f, 0.0f, 1.0f };

        /**
         * @brief A coefficient for ambient lighting
         */
        vec4 kAmbient{ 1.0f };

        /**
         * @brief A coefficient for diffuse lighting
         */
        vec4 kDiffuse{ 1.0f };

        /**
         * @brief A coefficient for specular lighting
         *
         * Determines the color of specular highlights on the lighted
         * surface.
         */
        vec4 kSpecular{ 1.0f };

        /**
         * @brief The sharpness of specular reflections
         */
        float shininess{ 1.0f };

        /**
         * @brief Reflectivity, in the range [0, 1]
         *
         * Ray-tracing-only property.
         */
        float reflectivity{ 0.0f };

        /**
         * @brief A color texture
         */
        ui32 diffuseTexture{ NO_TEXTURE };

        /**
         * @brief A specular texture
         *
         * Determines the strength of specular highlights. Has only one
         * channel.
         */
        ui32 specularTexture{ NO_TEXTURE };

        /**
         * @brief A bump/normal texture
         *
         * Contains modified surface normals for the shaded object.
         */
        ui32 bumpTexture{ NO_TEXTURE };

        bool32 performLighting{ true };

        ui32 __padding[2]{ 0, 0 };
    };

    static_assert(util::sizeof_pad_16_v<Material> == sizeof(Material),
                  "The Material struct must always be padded to 16 bytes");
} // namespace trc
