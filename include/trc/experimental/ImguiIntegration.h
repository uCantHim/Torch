#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
namespace ig = ImGui;
#include <vkb/basics/Swapchain.h>

#include "Renderer.h"
#include "Scene.h"

namespace trc::experimental
{
    extern auto getImguiRenderStageType() -> RenderStageType::ID;
    extern auto getImguiRenderPass(const vkb::Swapchain& swapchain) -> RenderPass::ID;
    extern auto getImguiPipeline() -> Pipeline::ID;

    /**
     * @brief RenderPass for ImGui
     *
     * Renders to the swapchain, but after the final lighting pass. Meaning
     * it overwrites everything else.
     *
     * It is guaranteed that the ImGui render functions are executed after
     * all other draw functions that are attached to this renderpass/-stage.
     */
    class ImGuiRenderPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 1;

        explicit ImGuiRenderPass(const vkb::Swapchain& swapchain);
        ~ImGuiRenderPass() override;

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

    private:
        struct CallbackStorage
        {
            GLFWcharfun vkbCharCallback;
            GLFWkeyfun vkbKeyCallback;
            GLFWmousebuttonfun vkbMouseButtonCallback;
            GLFWscrollfun vkbScrollCallback;
        };

        static inline std::unordered_map<const GLFWwindow*, CallbackStorage> callbackStorages;

        const vkb::Swapchain& swapchain;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };

    extern void initImgui(const vkb::Device& device,
                          Renderer& renderer,
                          const vkb::Swapchain& swapchain);

    extern void terminateImgui();

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
