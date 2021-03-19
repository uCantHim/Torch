#pragma once

#include <vector>

#include "Transform.h"

namespace trc::ui
{
    template<typename Derived>
    class CRTPNode
    {
    public:
        CRTPNode(const CRTPNode&) = delete;
        auto operator=(const CRTPNode&) = delete;

        CRTPNode() {
            static_assert(std::is_base_of_v<CRTPNode<Derived>, Derived>, "");
        }
        CRTPNode(CRTPNode&&) noexcept = default;
        ~CRTPNode();

        auto operator=(CRTPNode&&) noexcept -> CRTPNode& = default;

        void attach(Derived& child);
        void detach(Derived& child);
        void clearChildren();

        template<std::invocable<Derived&> F>
        void foreachChild(F func);

    private:
        Derived* parent{ nullptr };
        std::vector<Derived*> children;
    };

    /**
     * Idea:
     *
     * Global positions don't exist. They are calculated exclusively for
     * internal calculations during drawing.
     */
    template<typename Derived>
    class TransformNode : public CRTPNode<Derived>
    {
    public:
        /**
         * Can't use concepts with CRTP because neither the derived type
         * nor the parent type are complete at concept resolution time, so
         * I use static_assert instead.
         */
        TransformNode() {
            static_assert(std::is_base_of_v<TransformNode<Derived>, Derived>, "");
        }

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

    private:
        Transform localTransform;
    };

#include "CRTPNode.inl"

} // namespace trc::ui
