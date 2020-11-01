#pragma once

#include "base/SceneBase.h"
#include "Node.h"
#include "Drawable.h"

namespace trc::experimental
{
    class DrawableInterface
    {
    public:
        virtual ~DrawableInterface() = default;

        virtual void attachToScene(SceneBase& scene) = 0;
        virtual void removeFromScene() = 0;
    };

    /**
     * A DrawableType is defined as being attachable to and removable from
     * a scene.
     */
    template<typename T>
    concept DrawableType = requires (T a, SceneBase scene) {
        { a.attachToScene(scene) };
        { a.removeFromScene() };
    };

    /**
     * A DrawableNodeType is a DrawableType that also inherits from Node.
     */
    template<typename T>
    concept DrawableNodeType = requires {
        DrawableType<T>;
        std::is_base_of_v<Node, T>;
    };

    /**
     * @brief Implements the virtual DrawableInterface for any DrawableType
     *
     * If you also want to have access to a drawable type's Node interface,
     * use DrawableChainRoot from `<trc/experimental/DrawableChain.h>`.
     */
    template<DrawableType T>
    class VirtualDrawable : public DrawableInterface, public T
    {
    public:
        VirtualDrawable() requires std::is_default_constructible_v<T>
            = default;

        template<typename... Args>
        explicit VirtualDrawable(Args&&... args)
            : T(std::forward<Args>(args)...)
        {}

        inline void attachToScene(SceneBase& scene) override {
            T::attachToScene(scene);
        }

        inline void removeFromScene() override {
            T::removeFromScene();
        }
    };
} // namespace trc::experimental
