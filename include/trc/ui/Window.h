#pragma once

#include "trc/text/GlyphLoading.h"
#include "trc/ui/Element.h"
#include "trc/ui/FontRegistry.h"
#include "trc/ui/IoConfig.h"

namespace trc::ui
{
    class Window;

    /**
     * Typed unique handle for UI elements
     */
    template<GuiElement E>
    using SharedElementHandle = s_ptr<E>;

    /**
     * Typed shared handle for UI elements
     */
    template<GuiElement E>
    using UniqueElementHandle = u_ptr<E, std::function<void(E*)>>;

    /**
     * Temporary proxy that creates either an unmanaged reference or a
     * smart handle (i.e. unique or shared).
     */
    template<GuiElement E>
    class ElementHandleProxy
    {
        friend class Window;
        ElementHandleProxy(E& element, Window& window);

    public:
        using SharedHandle = SharedElementHandle<E>;
        using UniqueHandle = UniqueElementHandle<E>;

        ElementHandleProxy(const ElementHandleProxy<E>&) = delete;
        ElementHandleProxy(ElementHandleProxy<E>&&) noexcept = delete;
        auto operator=(const ElementHandleProxy<E>&) -> ElementHandleProxy<E>& = delete;
        auto operator=(ElementHandleProxy<E>&&) noexcept -> ElementHandleProxy<E>& = delete;

        inline operator E&() &&;
        inline auto makeRef() && -> E&;
        inline auto makeShared() && -> SharedHandle;
        inline auto makeUnique() && -> UniqueHandle;

    private:
        E* element;
        Window* window;
    };

    /**
     * @brief Initialize global callbacks to the user
     *
     * The user is notified when a resource is loaded, for example. These
     * callbacks are usually set by the active backend that has to manage
     * its versions of loaded resources.
     */
    void initUserCallbacks(std::function<void(ui32, const GlyphCache&)> onFontLoad,
                           std::function<void(ui32)>                    onImageLoad);

    class WindowBackend
    {
    public:
        virtual ~WindowBackend() = default;

        virtual auto getSize() -> vec2 = 0;
    };

    struct WindowCreateInfo
    {
        u_ptr<WindowBackend> windowBackend;
        std::function<void(Window&)> onWindowDestruction{ [](auto&&) {} };

        KeyMapping keyMap;
    };

    /**
     * Acts as a root for the gui
     */
    class Window
    {
    public:
        explicit Window(WindowCreateInfo createInfo);
        Window(Window&&) noexcept = default;
        ~Window();

        Window() = delete;
        Window(const Window&) = delete;

        auto operator=(const Window&) -> Window& = delete;
        auto operator=(Window&&) -> Window& = delete;

        /**
         * Calculate global transformatio, then build a list of DrawInfos
         * from all elements in the tree.
         */
        auto draw() -> const DrawList&;

        auto getSize() const -> vec2;
        auto getRoot() -> Element&;

        /**
         * @brief Create an element
         */
        template<GuiElement E, typename... Args>
        inline auto create(Args&&... args) -> ElementHandleProxy<E>;

        /**
         * @brief Destroy an element
         *
         * @param Element& elem The element to destroy. Must have been
         *        created at the same window.
         */
        void destroy(Element& elem);

        /**
         * @brief Signal to the window that a mouse click has occured
         */
        void signalMouseClick(float posPixelsX, float posPixelsY);

        // void signalMouseRelease(float posPixelsX, float posPixelsY);
        // void signalMouseMove(float posPixelsX, float posPixelsY);

        void signalKeyPress(int key);
        void signalKeyRepeat(int key); // Just issues a key press event for now
        void signalKeyRelease(int key);
        void signalCharInput(ui32 character);

        auto getIoConfig() -> IoConfig&;
        auto getIoConfig() const -> const IoConfig&;

        /**
         * Calculate the absolute pixel values of a normalized point
         */
        auto normToPixels(vec2 p) const -> vec2;

        /**
         * Normalize a point in pixels relative to the window size
         */
        auto pixelsToNorm(vec2 p) const -> vec2;

        /**
         * @brief Called when the window is destroyed
         */
        std::function<void(Window&)> onWindowDestruction{ [](auto&&) {} };

    private:
        /**
         * @brief Dispatch an event to all events that the mouse hovers
         */
        template<std::derived_from<event::MouseEvent> EventType>
        void descendMouseEvent(EventType event);

        /**
         * Stops descending the event if `breakCondition` returns `false`.
         */
        template<
            std::derived_from<event::EventBase> EventType,
            typename F
        >
        void descendEvent(EventType event, F breakCondition)
            requires std::is_same_v<bool, std::invoke_result_t<F, Element&>>;

        /**
         * @brief Recalculate positions of elements
         *
         * This converts normalized or absolute positions based on window size
         * and stuff like that.
         */
        void realignElements();

        u_ptr<WindowBackend> windowBackend;
        IoConfig ioConfig;

        std::vector<u_ptr<Element>> drawableElements;
        DrawList drawList;

        /**
         * Traverses the tree recusively and applies a function to all
         * visited elements. The function is applied to parents first, then
         * to their children.
         */
        template<std::invocable<Element&> F>
        void traverse(F elemCallback);

        /**
         * @brief An element that does nothing
         */
        struct Root : Element {
            void draw(DrawList&) override {}
        };

        // This is a unique_ptr because I'm too lazy to implement all those
        // move constructors.
        u_ptr<Root> root{ new Root };
    };

} // namespace trc::ui

#include "trc/ui/Window.inl"
