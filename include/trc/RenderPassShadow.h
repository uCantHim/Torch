#pragma once

#include <vector>

#include <vkb/Image.h>

#include "Boilerplate.h"
#include "Node.h"
#include "RenderPass.h"

namespace trc
{
    /**
     * Created and owned by RenderPassShadow.
     */
    class ShadowSource : public Node
    {
    public:
        void setProjectionMatrix(const mat4& proj);
        void setImageSize(uvec2 size);
    };


    class RenderPassShadow : public RenderPass
    {
    public:
        RenderPassShadow();

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getShadowSource() -> ShadowSource&;

    private:
        // unique ptr to keep outgoing references alive after copy
        std::unique_ptr<ShadowSource> source;
    };


    class ShadowDescriptor
    {
    public:
        static void addShadow(vk::Image image, const mat4& viewMatrix);

    private:
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vkb::FrameSpecificObject<vk::UniqueDescriptorSet> descSet;
    };
} // namespace trc
