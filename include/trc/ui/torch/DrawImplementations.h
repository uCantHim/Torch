#pragma once

#include <vkb/Buffer.h>

#include "Pipeline.h"
#include "data_utils/IndexMap.h"
#include "text/GlyphMap.h"
#include "ui/torch/DynamicBuffer.h"

#include "ui/DrawInfo.h"
#include "ui/Font.h"

namespace trc::ui_impl
{
    class DrawCollector
    {
    public:
        DrawCollector(const vkb::Device& device, vk::RenderPass renderPass);
        ~DrawCollector() = default;

        void beginFrame(vec2 windowSizePixels);
        void drawElement(const ui::DrawInfo& info);
        void endFrame(vk::CommandBuffer cmdBuf);

    private:
        static void initStaticResources(const vkb::Device& device, vk::RenderPass renderPass);
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static auto makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline::ID;
        static auto makeTextPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline::ID;

        void add(const ui::ElementDrawInfo& elem, const ui::types::NoType&);
        void add(const ui::ElementDrawInfo& elem, const ui::types::Text&);

        // General resources
        const vkb::Device& device;
        vkb::DeviceLocalBuffer quadVertexBuffer;
        vec2 windowSizePixels;

        void createDescriptorSet(const vkb::Device& device);
        void updateFontDescriptor();
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;

        // Plain quad resources
        struct QuadData
        {
            vec2 pos;
            vec2 size;

            vec4 color;
        };
        DynamicBuffer<QuadData> quadBuffer;

        // Text resources
        struct FontInfo
        {
        public:
            explicit FontInfo(const vkb::Device& device);

            auto getGlyphUvs(wchar_t character) -> GlyphMap::UvRectangle;

            ui32 fontIndex;
            std::unique_ptr<GlyphMap> glyphMap;
            vk::UniqueImageView imageView;
            data::IndexMap<wchar_t, std::pair<bool, GlyphMap::UvRectangle>> glyphTextureCoords;
        };

        /**
         * @brief Add a font
         *
         * @param ui32 fontIndex The font's index in the ui::FontRegistry
         */
        void addFont(ui32 fontIndex);
        std::unordered_map<ui32, FontInfo> fonts;

        struct LetterData
        {
            vec2 basePos; // Base position of the whole text
            vec2 pos;
            vec2 size;

            vec2 texCoordLL; // lower left texture coordinate
            vec2 texCoordUR; // upper right texture coordinate

            float bearingY;
            ui32 fontIndex;

            vec4 color;
        };
        DynamicBuffer<LetterData> letterBuffer;
    };
} // namespace trc::ui_impl
