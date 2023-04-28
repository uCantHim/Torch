#pragma once

#include <map>

#include <trc_util/data/IndexMap.h>
#include "trc/base/Buffer.h"

#include "trc/core/Pipeline.h"
#include "trc/text/GlyphMap.h"
#include "trc/ui/DrawInfo.h"
#include "trc/ui/FontRegistry.h"
#include "trc/ui/torch/DynamicBuffer.h"

namespace trc {
    class GuiRenderer;
}

namespace trc::ui_impl
{
    using namespace trc::basic_types;

    class DrawCollector
    {
    public:
        DrawCollector(const Device& device, ::trc::GuiRenderer& renderer);
        ~DrawCollector();

        void beginFrame();
        void drawElement(const ui::DrawInfo& info);
        void endFrame(vk::CommandBuffer cmdBuf, uvec2 windowSizePixels);

    private:
        static void initStaticResources();

        // A list of all existing collectors to notify them about newly loaded resources
        static inline std::vector<DrawCollector*> existingCollectors;
        // Save previously loaded fonts for collectors constructed later on
        static inline std::vector<std::pair<ui32, const GlyphCache&>> existingFonts;

        auto makeLinePipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline;
        auto makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline;
        auto makeTextPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline;

        /** @brief Internal drawable type representing a border around an element */
        struct _border {};

        void add(vec2 pos, vec2 size, const ui::ElementStyle& elem, const ui::types::NoType&);
        void add(vec2 pos, vec2 size, const ui::ElementStyle& elem, const ui::types::Line&);
        void add(vec2 pos, vec2 size, const ui::ElementStyle& elem, const ui::types::Quad&);
        void add(vec2 pos, vec2 size, const ui::ElementStyle& elem, const ui::types::Text&);
        void add(vec2 pos, vec2 size, const ui::ElementStyle& elem, _border);

        // General resources
        const Device& device;
        DeviceLocalBuffer quadVertexBuffer;
        DeviceLocalBuffer lineUvBuffer{
            device,
            std::vector<vec2>{ vec2(0.0f, 0.0f), vec2(1.0f, 1.0f) },
            vk::BufferUsageFlagBits::eVertexBuffer
        };

        // Descriptors
        void updateFontDescriptor();
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet fontDescSet;

        // Pipelines
        trc::PipelineLayout linePipelineLayout;
        trc::PipelineLayout quadPipelineLayout;
        trc::PipelineLayout textPipelineLayout;
        trc::Pipeline linePipeline;
        trc::Pipeline quadPipeline;
        trc::Pipeline textPipeline;

        // Plain line vertices
        struct Line
        {
            vec2 start;
            vec2 end;

            vec4 color;
            float width;
        };
        std::vector<Line> lines;

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
            explicit FontInfo(const Device& device, ui32 fontIndex, const GlyphCache& cache);

            auto getGlyphUvs(wchar_t character) -> GlyphMap::UvRectangle;

            ui32 fontIndex;
            const GlyphCache& glyphCache;
            std::unique_ptr<GlyphMap> glyphMap;
            vk::UniqueImageView imageView;
            data::IndexMap<wchar_t, std::pair<bool, GlyphMap::UvRectangle>> glyphTextureCoords;
        };

        /**
         * @brief Add a font
         *
         * @param ui32 fontIndex The font's index in the ui::FontRegistry
         */
        void addFont(ui32 fontIndex, const GlyphCache& glyphCache);
        std::map<ui32, FontInfo> fonts; // std::map is terrible, but IndexMap is not iterable

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
        struct TextRange
        {
            vec2 scissorOffset;
            vec2 scissorSize;
            ui32 numLetters;
        };
        std::vector<TextRange> textRanges;
        DynamicBuffer<LetterData> letterBuffer;
    };
} // namespace trc::ui_impl
