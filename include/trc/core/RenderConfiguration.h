#pragma once

#include <concepts>
#include <unordered_map>

#include "DescriptorRegistry.h"
#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "RenderLayout.h"

namespace trc
{
    struct DrawConfig;
    class RenderTarget;

    /**
     * @brief Strong type to reference a render pass
     */
    struct RenderPassName
    {
        std::string identifier;
    };

    class RenderPassRegistry
    {
    public:
        struct RenderPassDefinition
        {
            vk::RenderPass pass;
            ui32 subpass;
        };

        using RenderPassGetter = std::function<RenderPassDefinition()>;

        void addRenderPass(RenderPassName name, RenderPassGetter getter);

        auto getRenderPass(const RenderPassName& name) const
            -> RenderPassDefinition;

    private:
        std::unordered_map<std::string, RenderPassGetter> renderPasses;
    };

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig : public RenderPassRegistry
                       , public DescriptorRegistry
    {
    public:
        explicit RenderConfig(RenderLayout layout);
        virtual ~RenderConfig() = default;

        virtual void preDraw(const DrawConfig& draw) = 0;
        virtual void postDraw(const DrawConfig& draw) = 0;

        virtual void setViewport(uvec2 newOffset, uvec2 newSize) = 0;
        virtual void setRenderTarget(const RenderTarget& newTarget) = 0;

        virtual auto getPipeline(Pipeline::ID id) -> Pipeline& = 0;

        auto getLayout() -> RenderLayout&;

    protected:
        RenderLayout layout;
    };

    /**
     * @brief A type that implements RenderConfig
     */
    template<typename T>
    concept RenderConfigType = std::derived_from<T, RenderConfig>;
} // namespace trc
