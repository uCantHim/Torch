#pragma once

#include <vkb/Buffer.h>

#include "Pipeline.h"
#include "text/Font.h"
#include "ui/DrawInfo.h"

namespace trc::ui_impl
{
    template<typename T>
    struct DynamicBuffer
    {
    public:
        DynamicBuffer(const vkb::Device& device,
                      size_t initialSize,
                      vk::BufferUsageFlags usageFlags,
                      vk::MemoryPropertyFlags memoryProperties)
            :
            buffer(device, initialSize * sizeof(T), usageFlags, memoryProperties),
            mappedBuf(reinterpret_cast<T*>(buffer.map()))
        {}

        inline auto operator*() const -> vk::Buffer {
            return *buffer;
        }

        inline void push(T val)
        {
            mappedBuf[currentOffset] = val;
            currentOffset++;
        }

        inline void reset()
        {
            currentOffset = 0;
        }

        inline auto size() const -> ui32
        {
            return currentOffset;
        }

    private:
        vkb::Buffer buffer;
        T* mappedBuf;
        ui32 currentOffset{ 0 };
    };

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
