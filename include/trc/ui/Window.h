#pragma once

#include "Element.h"
#include "Font.h"
#include "text/GlyphLoading.h"

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

    class WindowBackend
    {
    public:
        virtual auto getSize() -> vec2 = 0;

        virtual void uploadImage(const fs::path& imagePath) = 0;
        virtual void uploadFont(const fs::path& fontPath) = 0;
    };

    struct WindowCreateInfo
    {
        u_ptr<WindowBackend> windowProvider;
    };

    /**
     * Acts as a root for the gui
     */
    class Window
    {
    public:
        explicit Window(WindowCreateInfo createInfo);

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
            requires std::is_constructible_v<E, Args...>
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

        u_ptr<WindowBackend> windowInfo;

        std::vector<u_ptr<Element>> drawableElements;
        DrawList drawList;

        /**
         * Traverses the tree recusively and applies a function to all
         * visited elements. The function is applied to parents first, then
         * to their children.
         */
        template<std::invocable<Element&, vec2, vec2> F>
        inline void traverse(F elemCallback);

        /**
         * @brief An element that does nothing
         */
        struct Root : Element {
            void draw(DrawList&, vec2, vec2) override {}
        };

        Root root;
    };

#include "Window.inl"

} // namespace trc::ui
