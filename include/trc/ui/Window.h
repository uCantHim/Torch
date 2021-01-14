#pragma once

#include "Element.h"
#include "UiRenderer.h"

namespace trc::ui
{
    auto calcGlobalTransform(const Node& node);

    class WindowInformationProvider
    {

    };

    //template<GuiElement E>
    //class UniqueElementHandle
    //{
    //    friend class Window;
    //    UniqueElementHandle(E& elem, Window& window);

    //public:
    //    UniqueElementHandle(const UniqueElementHandle<E>&) = delete;
    //    auto operator=(const UniqueElementHandle<E>&) -> UniqueElementHandle<E>& = delete;

    //    UniqueElementHandle(UniqueElementHandle<E>&&) noexcept = default;
    //    ~UniqueElementHandle() = default;

    //    auto operator=(UniqueElementHandle<E>&&) noexcept -> UniqueElementHandle<E>& = default;

    //    inline auto operator->() -> E*;
    //    inline auto operator->() const -> const E*;

    //private:
    //    using Deleter = std::function<void(E*)>;
    //    std::unique_ptr<E, Deleter> ptr;
    //};

    //template<GuiElement E>
    //class SharedElementHandle
    //{
    //    friend class Window;
    //    SharedElementHandle(E& elem, Window& window);

    //public:
    //    SharedElementHandle(const SharedElementHandle<E>&) = default;
    //    SharedElementHandle(SharedElementHandle<E>&&) noexcept = default;
    //    ~SharedElementHandle() = default;

    //    auto operator=(const SharedElementHandle<E>&) -> SharedElementHandle<E>& = default;
    //    auto operator=(SharedElementHandle<E>&&) noexcept -> SharedElementHandle<E>& = default;

    //    auto operator->() -> E*;
    //    auto operator->() const -> const E*;
    //};

    template<GuiElement E>
    using SharedElementHandle = std::shared_ptr<E>;
    template<GuiElement E>
    using UniqueElementHandle = std::unique_ptr<E, std::function<void(E*)>>;

    template<GuiElement E>
    class ElementHandleFactory
    {
        friend class Window;
        explicit ElementHandleFactory(E& element, Window& window);

    public:
        using SharedHandle = SharedElementHandle<E>;
        using UniqueHandle = UniqueElementHandle<E>;

        inline operator E&() &&;
        inline auto makeShared() && -> SharedHandle;
        inline auto makeUnique() && -> UniqueHandle;

    private:
        E* element;
        Window* window;
    };

    /**
     * Acts as a root for the gui
     */
    class Window
    {
    public:
        Window(WindowInformationProvider window, std::unique_ptr<Renderer> renderer);

        /**
         * Realign elements, then draw then to a framebuffer
         */
        void draw();

        auto getSize();

        template<GuiElement E, typename... Args>
            requires std::is_constructible_v<E, Args...>
        auto create(Args&&... args) -> ElementHandleFactory<E>;

        void destroy(Element& elem);

    private:
        /**
         * @brief Recalculate positions of elements
         *
         * This converts normalized or absolute positions based on window size
         * and stuff like that.
         */
        void realign();

        WindowInformationProvider window;
        std::vector<std::unique_ptr<Element>> drawableElements;

        std::unique_ptr<Renderer> renderer;
    };

#include "Window.inl"

} // namespace trc::ui
