#pragma once

#include <vector>
#include <concepts>

#include "Transform.h"

namespace trc::ui
{
    /**
     * Idea:
     *
     * Global positions don't exist. They are calculated exclusively for
     * internal calculations during drawing.
     */
    template<typename Derived>
    class CRTPNode
    {
    public:
        /**
         * Can't use concepts with CRTP because neither the derived type
         * not the parent type are defined, so I use static_assert.
         */
        CRTPNode() {
            static_assert(std::is_base_of_v<CRTPNode<Derived>, Derived>, "");
        }
        ~CRTPNode();

        /**
         * TODO: Implement all special member functions
         */

        auto getPos() -> vec2;
        auto getSize() -> vec2;
        void setPos(vec2 newPos);
        void setSize(vec2 newSize);

        auto getTransform() -> Transform;
        void setTransform(Transform newTransform);

        auto getPositionProperties() -> Transform::Properties;
        auto getSizeProperties() -> Transform::Properties;
        auto setPositionProperties(Transform::Properties newProps);
        auto setSizeProperties(Transform::Properties newProps);

        void attach(Derived& child);
        void detach(Derived& child);
        void clearChildren();

        template<std::invocable<Derived&> F>
        void foreachChild(F func);

    private:
        Transform localTransform;

        Derived* parent { nullptr };
        std::vector<Derived*> children;
    };

#include "CRTPNode.inl"

} // namespace trc::ui
