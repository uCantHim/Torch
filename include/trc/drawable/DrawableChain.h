#pragma once

#include "util/functional/Maybe.h"

#include "VirtualDrawable.h"

namespace trc
{
    class DrawableChainElement : public DrawableInterface, public Node
    {
    public:
        /**
         * From any drawable type
         */
        template<DrawableNodeType T> requires (!std::is_base_of_v<DrawableInterface, T>)
        explicit DrawableChainElement(T drawableObject)
            : DrawableChainElement(std::move(drawableObject), nullptr)
        {}

        /**
         * From any drawable type and decorated element as value
         */
        template<DrawableNodeType T> requires (!std::is_base_of_v<DrawableInterface, T>)
        DrawableChainElement(T drawableObject, DrawableChainElement decoratedObject)
            :
            DrawableChainElement(
                std::make_unique<VirtualDrawable<T>>(std::move(drawableObject)),
                std::make_unique<DrawableChainElement>(std::move(decoratedObject))
            )
        {}

        /**
         * From any drawable type and a decorated element as unique_ptr
         */
        template<DrawableNodeType T> requires (!std::is_base_of_v<DrawableInterface, T>)
        DrawableChainElement(T drawableObject,
                             std::unique_ptr<DrawableChainElement> decoratedObject)
            :
            DrawableChainElement(
                std::make_unique<VirtualDrawable<T>>(std::move(drawableObject)),
                std::move(decoratedObject)
            )
        {}

        /**
         * From drawable interface and optional decorated element, both as
         * unique_ptr.
         *
         * This is the base constructor that all other constructors call.
         */
        template<DrawableNodeType T> requires std::is_base_of_v<DrawableInterface, T>
        explicit DrawableChainElement(
            std::unique_ptr<T> drawableObject,
            std::unique_ptr<DrawableChainElement> decoratedObject = nullptr)
            :
            nextDecorated(std::move(decoratedObject))
        {
            // The drawable member must never be nullptr
            assert(drawableObject != nullptr);

            attach(*drawableObject);
            if (nextDecorated != nullptr)
            {
                // Attach the next chain element to the drawable instead of
                // *this because it should be influenced by the drawable's
                // transformation. This is consistent with the behaviour of
                // DrawableChainRoot<>.
                drawableObject->attach(*nextDecorated);
            }

            // Assign drawable now instead of via direct member initializer
            // because I need the type information for Node::attach above.
            drawable = std::move(drawableObject);
        }

        DrawableChainElement(DrawableChainElement&& other) = default;
        ~DrawableChainElement() = default;
        auto operator=(DrawableChainElement&& other) -> DrawableChainElement& = default;

        DrawableChainElement(const DrawableChainElement&) = delete;
        auto operator=(const DrawableChainElement&) -> DrawableChainElement& = delete;

        /**
         * @brief Attach all drawables in the chain to a scene
         */
        void attachToScene(SceneBase& scene) override;

        /**
         * @brief Remove all drawables in the chain from a scene
         */
        void removeFromScene() override;

        auto getDrawable() noexcept -> DrawableInterface&;
        auto getDrawable() const noexcept -> const DrawableInterface&;
        auto getNextChainElement() noexcept -> Maybe<DrawableChainElement&>;
        auto getNextChainElement() const noexcept -> Maybe<const DrawableChainElement&>;

    private:
        // Builds a chain of nodes that each keep one child alive
        std::unique_ptr<DrawableChainElement> nextDecorated{ nullptr };

        // Contents of DrawableChainElement here
        std::unique_ptr<DrawableInterface> drawable;
    };

    /**
     * @brief A typesafe root for a chain of drawables
     *
     * Similar to DrawableChainElement, but inherits from the drawable type
     * instead of storing it as a component. The DrawableChainRoot thus
     * *is* the top-level drawable on the chain.
     *
     * This class does the following:
     *  - Inherit from the template parameter type T
     *  - Store a DrawableChainElement
     *  - Provide a DrawableChainElement-like interface to said stored
     *    element.
     *
     * A classical use case for this is when you want to have the full
     * interface of trc::Drawable (for example for access to the animation
     * engine), but also the ability to attach multiple drawables to one
     * another:
     *
     *     using MyDrawable = trc::DrawableChainRoot<trc::Drawable>;
     *     struct MyEntity
     *     {
     *         MyDrawable drawable;
     *         // ...
     *     };
     */
    template<DrawableNodeType T>
    class DrawableChainRoot : public T
    {
    public:
        template<typename... Args>
        explicit DrawableChainRoot(Args&&... args)
            requires std::is_constructible_v<T, Args...>
            :
            DrawableChainRoot(nullptr, std::forward<Args>(args)...)
        {}

        template<typename... Args>
        explicit DrawableChainRoot(DrawableChainElement nextElem, Args&&... args)
            requires std::is_constructible_v<T, Args...>
            :
            DrawableChainRoot(
                std::make_unique<DrawableChainElement>(std::move(nextElem)),
                std::forward<Args>(args)...
            )
        {}

        template<typename... Args>
        explicit DrawableChainRoot(std::unique_ptr<DrawableChainElement> nextElem, Args&&... args)
            requires std::is_constructible_v<T, Args...>
            :
            T(std::forward<Args>(args)...),
            nextChainElement(std::move(nextElem))
        {
            if (nextChainElement != nullptr) {
                T::attach(*nextChainElement); // Because T must be a DrawableNodeType
            }
        }

        DrawableChainRoot(DrawableChainRoot&& other) noexcept = default;
        ~DrawableChainRoot() = default;
        auto operator=(DrawableChainRoot&& other) noexcept -> DrawableChainRoot& = default;

        DrawableChainRoot(const DrawableChainRoot&) = delete;
        auto operator=(const DrawableChainRoot&) -> DrawableChainRoot& = delete;

        /**
         * @brief Attach all drawables in the chain to a scene
         */
        inline void attachToScene(SceneBase& scene)
        {
            T::attachToScene(scene);
            if (nextChainElement) {
                nextChainElement->attachToScene(scene);
            }
        }

        /**
         * @brief Remove all drawables in the chain from a scene
         */
        inline void removeFromScene()
        {
            T::removeFromScene();
            if (nextChainElement) {
                nextChainElement->removeFromScene();
            }
        }

        inline auto getNextChainElement() noexcept -> Maybe<DrawableChainElement&>
        {
            if (nextChainElement != nullptr) {
                return *nextChainElement;
            }
            return {};
        }

        inline auto getNextChainElement() const noexcept -> Maybe<const DrawableChainElement&>
        {
            if (nextChainElement != nullptr) {
                return *nextChainElement;
            }
            return {};
        }

    private:
        std::unique_ptr<DrawableChainElement> nextChainElement;
    };

    /**
     * @brief Helper to create chains of drawables quickly
     *
     * This overload basically wraps a drawable type in a virtual interface
     * that exposes the DrawableInterface as well as a Node interface.
     *
     * Call this function with with multiple arguments to actually chain
     * more than one drawable together.
     */
    template<DrawableNodeType T>
    inline auto makeDrawableChain(T&& first) -> DrawableChainElement
    {
        return DrawableChainElement{ std::forward<T>(first) };
    }

    /**
     * @brief Helper to create chains of drawables quickly
     *
     * Creates a drawable chain of arbitrary length.
     */
    template<DrawableNodeType T, DrawableNodeType... Args>
    inline auto makeDrawableChain(T&& first, Args&&... args) -> DrawableChainElement
    {
        return DrawableChainElement{
            std::forward<T>(first),
            makeDrawableChain(std::forward<Args>(args)...)
        };
    }

    /**
     * @brief Helper to create chains of drawables quickly
     *
     * Creates a unique_ptr to a drawable chain of arbitrary length.
     */
    template<DrawableNodeType... Args>
    inline auto makeDrawableChainUnique(Args&&... args) -> std::unique_ptr<DrawableChainElement>
    {
        return std::make_unique<DrawableChainElement>(
            makeDrawableChain(std::forward<Args>(args)...)
        );
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a root with an attached drawable chain of arbitrary length.
     * The first argument is the root itself.
     */
    template<DrawableNodeType T>
    inline auto makeDrawableChainRoot(T&& first) -> DrawableChainRoot<std::decay_t<T>>
    {
        return DrawableChainRoot<std::decay_t<T>>{ std::move(first) };
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a root with an attached drawable chain of arbitrary length.
     * The first argument is the root itself.
     */
    template<DrawableNodeType T, DrawableNodeType... Args>
    inline auto makeDrawableChainRoot(T&& first, Args&&... args)
        -> DrawableChainRoot<std::decay_t<T>>
    {
        return DrawableChainRoot<std::decay_t<T>>{
            makeDrawableChainUnique(std::forward<Args>(args)...),
            std::move(first)
        };
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a unique_ptr to root with an attached drawable chain of
     * arbitrary length. The first argument is the root itself.
     *
     * Overload for a single argument, i.e. a root with no attachments.
     */
    template<DrawableNodeType T>
    inline auto makeDrawableChainRootUnique(T&& first)
        -> std::unique_ptr<DrawableChainRoot<std::decay_t<T>>>
    {
        return std::make_unique<DrawableChainRoot<std::decay_t<T>>>(std::move(first));
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a unique_ptr to root with an attached drawable chain of
     * arbitrary length. The first argument is the root itself.
     */
    template<DrawableNodeType T, DrawableNodeType... Args>
    inline auto makeDrawableChainRootUnique(T&& first, Args&&... args)
        -> std::unique_ptr<DrawableChainRoot<std::decay_t<T>>>
    {
        return std::make_unique<DrawableChainRoot<std::decay_t<T>>>(
            makeDrawableChainUnique(std::forward<Args>(args)...),
            std::move(first)
        );
    }
} // namespace trc::experimental
