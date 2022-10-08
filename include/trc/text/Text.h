#pragma once

#include <string>

#include "trc/base/Buffer.h"

#include "trc/Node.h"
#include "trc/core/SceneBase.h"
#include "trc/text/Font.h"

namespace trc
{
    /**
     * @brief A 3D object that displays text
     */
    class Text : public Node
    {
    public:
        Text(const Instance& instance, FontHandle font);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void print(const std::string& str);

    private:
        // Scale the text down to get a crisp resolution
        static constexpr float BASE_SCALING{ 0.075f };

        const Instance& instance;
        FontHandle font;

        DeviceLocalBuffer vertexBuffer;
        SceneBase::UniqueRegistrationID drawRegistration;

        struct LetterData
        {
            vec2 texCoordLL;
            vec2 texCoordUR;
            vec2 glyphOffset; // Offset from text start
            vec2 glyphSize;   // Normalized glyph size
            float bearingY;
        };
        Buffer glyphBuffer;
        ui32 numLetters{ 0 };
    };
} // namespace trc
