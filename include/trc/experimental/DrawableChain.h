#pragma once

#include "utils/Maybe.h"
#include "VirtualDrawable.h"

namespace trc::experimental
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

            // Assign drawable now instead of via direct member initializer
            // because I need the type information for attach().
            attach(*drawableObject);
            drawable = std::move(drawableObject);

            if (decoratedObject != nullptr) {
                attach(*decoratedObject);
            }
        }

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
        auto getNextChainElement() noexcept -> Maybe<DrawableChainElement*>;
        auto getNextChainElement() const noexcept -> Maybe<const DrawableChainElement*>;

    private:
        // Builds a chain of nodes that each keep one child alive
        std::unique_ptr<DrawableChainElement> nextDecorated{ nullptr };

        // Contents of DrawableChainElement here
        std::unique_ptr<DrawableInterface> drawable;
    };

    /**
     * @brief A typesafe root for a chain of drawables
     *
     * Use this if you want to have a chain of drawables but also a
     * non-polymorphic interface of a specific drawable class. This class
     * simply inherits from the template parameter type.
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
    inline auto makeDrawableChainRoot(T&& first) -> DrawableChainRoot<T>
    {
        return DrawableChainRoot<T>{ std::move(first) };
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a root with an attached drawable chain of arbitrary length.
     * The first argument is the root itself.
     */
    template<DrawableNodeType T, DrawableNodeType... Args>
    inline auto makeDrawableChainRoot(T&& first, Args&&... args) -> DrawableChainRoot<T>
    {
        return DrawableChainRoot<T>{
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
    inline auto makeDrawableChainRootUnique(T&& first) -> std::unique_ptr<DrawableChainRoot<T>>
    {
        return std::make_unique<DrawableChainRoot<T>>(std::move(first));
    }

    /**
     * @brief Helper to create a root to a chain of drawables quickly
     *
     * Creates a unique_ptr to root with an attached drawable chain of
     * arbitrary length. The first argument is the root itself.
     */
    template<DrawableNodeType T, DrawableNodeType... Args>
    inline auto makeDrawableChainRootUnique(T&& first, Args&&... args)
        -> std::unique_ptr<DrawableChainRoot<T>>
    {
        return std::make_unique<DrawableChainRoot<T>>(
            makeDrawableChainUnique(std::forward<Args>(args)...),
            std::move(first)
        );
    }
} // namespace trc::experimental
