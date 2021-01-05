#pragma once

#include "ImguiIntegration.h"

namespace trc::experimental::imgui {

template<typename T>
concept ImguiDrawable = requires (T a) {
    { a.drawImgui() };
};

/**
 * @brief ImguiDrawable-satisfying wrapper for a std::function
 */
class ImguiFunction
{
public:
    ImguiFunction() = default;

    explicit ImguiFunction(std::function<void()> drawFunc)
        : func(std::move(drawFunc)) {}

    inline void drawImgui() {
        func();
    }

private:
    std::function<void()> func{ []{} };
};

/**
 * @brief Type-erasure for types that satisfy ImguiDrawable
 */
struct ImguiGeneric
{
public:
    ImguiGeneric(const ImguiGeneric&) = delete;
    ImguiGeneric& operator=(const ImguiGeneric&) = delete;

    // Defaults to an ImguiFunction with empty call operator
    ImguiGeneric()
        : ImguiGeneric(ImguiFunction([] {}))
    {}

    // Constructor from value
    template<ImguiDrawable T>
    explicit inline ImguiGeneric(T&& wrappedElem)
        : element(std::make_unique<ElementWrapper<T>>(std::forward<T>(wrappedElem)))
    {}

    // Defaulted move constructor
    ImguiGeneric(ImguiGeneric&&) noexcept = default;

    // Defaulted destructor
    ~ImguiGeneric() = default;

    // Defaulted move assignment
    ImguiGeneric& operator=(ImguiGeneric&&) noexcept = default;

    inline void drawImgui() {
        element->drawImgui();
    }

private:
    struct ElementInterface
    {
        virtual void drawImgui() = 0;
        virtual void swap(ElementInterface& other);
    };

    template<ImguiDrawable T>
    struct ElementWrapper : public ElementInterface
    {
        ElementWrapper(T wrappedElem) : wrapped(std::move(wrappedElem)) {}

        void drawImgui() override {
            wrapped.drawImgui();
        }
        T wrapped;
    };

private:
    std::unique_ptr<ElementInterface> element;
};

} // namespace trc::experimental::imgui
