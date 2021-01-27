#pragma once

#include <vkb/Buffer.h>

#include "Pipeline.h"
#include "text/Font.h"
#include "ui/DrawInfo.h"
#include "ui/torch/DynamicBuffer.h"

namespace trc::ui_impl
{
    class DrawCollector
    {
    public:
        DrawCollector(const vkb::Device& device, vk::RenderPass renderPass);
        ~DrawCollector() = default;

        void beginFrame();
        void drawElement(const ui::DrawInfo& info);
        void endFrame(vk::CommandBuffer cmdBuf);

    private:
        static void initStaticResources(vk::RenderPass renderPass);
        static auto makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline::ID;

        void add(const ui::ElementDrawInfo& elem, const ui::types::NoType&);
        void add(const ui::ElementDrawInfo& elem, const ui::types::Text&);

        // General resources
        vkb::DeviceLocalBuffer quadVertexBuffer;

        void createDescriptorSet(const vkb::Device& device);
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
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
        struct LetterData
        {
            vec2 pos;
            vec2 size;

            vec2 texCoordLL; // lower left texture coordinate
            vec2 texCoordUR; // upper right texture coordinate

            float bearingY;
            ui32 fontIndex;
        };
        DynamicBuffer<LetterData> letterBuffer;
    };
} // namespace trc::ui_impl
