#pragma once

#include <string>

#include <vkb/Buffer.h>

#include "Font.h"
#include "Pipeline.h"
#include "AssetIds.h"
#include "core/SceneBase.h"
#include "Node.h"

namespace trc
{
    /**
     * @brief A 3D object that displays text
     */
    class Text : public Node
    {
    public:
        explicit Text(Font& font);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void print(std::string_view str);

        static auto getPipeline() -> Pipeline::ID;

    private:
        // Scale the text down to get a crisp resolution
        static constexpr float BASE_SCALING{ 0.075f };

        static inline std::unique_ptr<vkb::DeviceLocalBuffer> vertexBuffer;
        static vkb::StaticInit _init;

        SceneBase::UniqueRegistrationID drawRegistration;

        Font* font;

        struct LetterData
        {
            vec2 texCoordLL;
            vec2 texCoordUR;
            vec2 glyphOffset; // Offset from text start
            vec2 glyphSize;   // Normalized glyph size
            float bearingY;
        };
        vkb::Buffer glyphBuffer;
        ui32 numLetters{ 0 };
    };

    class Renderer;
} // namespace trc
