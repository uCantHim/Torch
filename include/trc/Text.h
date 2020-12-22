#pragma once

#include <string>

#include <vkb/Buffer.h>

#include "Font.h"
#include "Pipeline.h"
#include "AssetIds.h"
#include "base/SceneBase.h"
#include "Node.h"

namespace trc
{
    class Text : public Node
    {
    public:
        explicit Text(Font& font);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void print(std::string_view str);

    private:
        // A basic scaling applied to the text at all times. Ensures
        // proper glyph resolution.
        static constexpr float BASE_SCALING{ 0.1f };

        static inline std::unique_ptr<vkb::DeviceLocalBuffer> vertexBuffer;
        static vkb::StaticInit _init;

        SceneBase* scene{ nullptr };
        SceneBase::RegistrationID drawRegistration;

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

    void makeTextPipeline(const Renderer& renderer);
} // namespace trc
