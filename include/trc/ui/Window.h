#pragma once

#include "Element.h"

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
    class ElementHandleFactory
    {
        friend class Window;
        ElementHandleFactory(E& element, Window& window);

    public:
        using SharedHandle = SharedElementHandle<E>;
        using UniqueHandle = UniqueElementHandle<E>;

        ElementHandleFactory(const ElementHandleFactory<E>&) = delete;
        ElementHandleFactory(ElementHandleFactory<E>&&) noexcept = delete;
        auto operator=(const ElementHandleFactory<E>&) -> ElementHandleFactory<E>& = delete;
        auto operator=(ElementHandleFactory<E>&&) noexcept -> ElementHandleFactory<E>& = delete;

        inline operator E&() &&;
        inline auto makeRef() && -> E&;
        inline auto makeShared() && -> SharedHandle;
        inline auto makeUnique() && -> UniqueHandle;

    private:
        E* element;
        Window* window;
    };

    class WindowInformationProvider
    {
    public:
        virtual auto getSize() -> vec2 = 0;
    };

    /**
     * Acts as a root for the gui
     */
    class Window
    {
    public:
        explicit Window(u_ptr<WindowInformationProvider> window);

        /**
         * Calculate global transformatio, then build a list of DrawInfos
         * from all elements in the tree.
         */
        auto draw() -> const DrawList&;

        auto getSize() -> vec2;
        auto getRoot() -> Element&;

        /**
         * @brief Create an element
         */
        template<GuiElement E, typename... Args>
            requires std::is_constructible_v<E, Args...>
        inline auto create(Args&&... args) -> ElementHandleFactory<E>;

        /**
         * @brief Destroy an element
         *
         * @param Element& elem The element to destroy. Must have been
         *        created at the same window.
         */
        void destroy(Element& elem);

        template<typename E>
        void notifyEvent(E event);

    private:
        auto concat(Transform parent, Transform child) -> Transform;

        /**
         * @brief Recalculate positions of elements
         *
         * This converts normalized or absolute positions based on window size
         * and stuff like that.
         */
        void realign();

        u_ptr<WindowInformationProvider> windowInfo;

        std::vector<u_ptr<Element>> drawableElements;
        DrawList drawList;

        /**
         * Traverses the tree recusively and calculates global transforms
         * of visited elements. Applies a function to all visited elements.
         */
        template<std::invocable<Element&, vec2, vec2> F>
        inline void traverse(F elemCallback);

        struct Root : Element {
            void draw(DrawList&, vec2, vec2) override {}
        };

        Root root;
    };

#include "Window.inl"

} // namespace trc::ui
