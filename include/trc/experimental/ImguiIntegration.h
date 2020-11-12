#pragma once

#include <trc/PipelineBuilder.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Renderer.h>
#include <trc/Scene.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
namespace ig = ImGui;

namespace trc::experimental
{
    constexpr ui32 IMGUI_RENDER_STAGE_TYPE_ID = 10;
    constexpr ui32 IMGUI_RENDER_STAGE_TYPE_PRIO = 10;
    constexpr ui32 IMGUI_RENDERPASS_INDEX = static_cast<ui32>(trc::internal::RenderPasses::NUM_PASSES) + 1;
    constexpr ui32 IMGUI_PIPELINE_INDEX = static_cast<ui32>(trc::internal::Pipelines::NUM_PIPELINES) + 1;

    /**
     * @brief Render stage specifically for ImGui
     *
     * Guide to use ImGui with torch:
     *
     *     size_t id = 10;        // Must be higher than 2
     *     size_t priority = 8;   // Must be higher than 0
     *
     *     trc::RenderStageType::create<trc::experimental::ImGuiRenderStageType>(id);
     *     renderer.enableRenderStageType(id, priority);
     *
     * Done.
     */
    class ImGuiRenderStage : public RenderStage
    {
    public:
        ImGuiRenderStage();
        ~ImGuiRenderStage();

    private:
        vk::UniqueDescriptorPool imGuiDescPool;
    };

    /**
     * @brief RenderPass for ImGui
     *
     * Renders the the swapchain, but after the final lighting pass. Meaning
     * it overwrites everything else.
     *
     * It is guaranteed that the ImGui render functions are executed after
     * all other draw functions that are attached to this renderpass/-stage.
     */
    class ImGuiRenderPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 1;

        ImGuiRenderPass();

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

    private:
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };

    namespace
    {
        // These are all used but the usage is not recognized
        [[maybe_unused]] static GLFWcharfun vkbCharCallback;
        [[maybe_unused]] static GLFWkeyfun vkbKeyCallback;
        [[maybe_unused]] static GLFWmousebuttonfun vkbMouseButtonCallback;
        [[maybe_unused]] static GLFWscrollfun vkbScrollCallback;
    }

    extern void initImgui(Renderer& renderer);

    /**
     * @brief Begin ImGui command recording
     *
     * The ImGui frame ends when the ImGuiRenderPass is executed. This
     * happens in Renderer::drawFrame.
     *
     * You can issue ImGui draw commands between the begin and end of the
     * ImGui frame. If you want to, you can call beginImgui() as the first
     * thing in your main loop.
     */
    extern void beginImgui();

    template<typename T>
    concept ImGuiDrawable = requires (T a) {
        { a.drawImGui() };
    };

    /**
     * @brief ImGui wrapper for a std::function
     */
    class ImGuiGenericElement
    {
    public:
        explicit ImGuiGenericElement(std::function<void()> drawFunc)
            : func(std::move(drawFunc)) {}

        inline void drawImGui() {
            func();
        }

    private:
        std::function<void()> func;
    };

    /**
     * @brief Root for any ImGuiDrawables
     */
    class ImGuiRoot
    {
    public:
        template<ImGuiDrawable ElemType, typename... Args>
        void registerElement(Args&&... args);

        void draw();

    private:
        /**
         * Any ImGuiDrawable. Satisfies ImGuiDrawable itself.
         */
        struct TypeErasedElement
        {
        public:
            // Constructor from value
            template<ImGuiDrawable T>
            TypeErasedElement(T wrappedElem)
                : element(std::make_unique<ElementWrapper<T>>(std::move(wrappedElem)))
            {}

            void drawImGui() {
                element->drawImGui();
            }

        private:
            struct ElementInterface {
                virtual void drawImGui() = 0;
            };

            template<ImGuiDrawable T>
            struct ElementWrapper : public ElementInterface
            {
                ElementWrapper(T wrappedElem) : wrapped(std::move(wrappedElem)) {}

                void drawImGui() override {
                    wrapped.drawImGui();
                }
                T wrapped;
            };

        private:
            std::unique_ptr<ElementInterface> element;
        };

        std::vector<std::unique_ptr<TypeErasedElement>> elements;
    };

    template<ImGuiDrawable ElemType, typename... Args>
    void ImGuiRoot::registerElement(Args&&... args)
    {
        elements.emplace_back(new TypeErasedElement(ElemType(std::forward<Args>(args)...)));
    }
} // namespace trc::experimental
