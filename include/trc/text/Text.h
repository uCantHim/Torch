#pragma once

#include <string>

#include <vkb/Buffer.h>

#include "core/SceneBase.h"
#include "core/Pipeline.h"
#include "AssetIds.h"
#include "Node.h"
#include "Font.h"

namespace trc
{
    /**
     * @brief A 3D object that displays text
     */
    class Text : public Node
    {
    public:
        Text(const Instance& instance, Font& font);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void print(std::string_view str);

        static auto getPipeline() -> Pipeline::ID;

    private:
        // Scale the text down to get a crisp resolution
        static constexpr float BASE_SCALING{ 0.075f };

        const Instance& instance;
        Font* font;

        vkb::DeviceLocalBuffer vertexBuffer;
        SceneBase::UniqueRegistrationID drawRegistration;

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
} // namespace trc
