#pragma once

#include <vector>

#include <trc_util/data/DeferredInsertVector.h>

#include "trc/ui/Transform.h"

namespace trc::ui
{
    template<typename Derived>
    class CRTPNode
    {
    public:
        CRTPNode(const CRTPNode&) = delete;
        CRTPNode(CRTPNode&&) noexcept = default;
        auto operator=(const CRTPNode&) = delete;
        auto operator=(CRTPNode&&) noexcept -> CRTPNode& = default;

        CRTPNode() {
            static_assert(std::is_base_of_v<CRTPNode<Derived>, Derived>, "");
        }

        ~CRTPNode() {
            foreachChild([](Derived& c){ c.parent = nullptr; });
            if (parent != nullptr) {
                parent->detach(static_cast<Derived&>(*this));
            }
        }

        void attach(Derived& child);
        void detach(Derived& child);

        template<std::invocable<Derived&> F>
        void foreachChild(F func);

    private:
        Derived* parent{ nullptr };
        data::DeferredInsertVector<Derived*> children;
    };

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
        void setPos(float x, float y);
        void setPos(pix_or_norm x, pix_or_norm y);
        void setPos(Vec2D<pix_or_norm> v);

        void setSize(vec2 newSize);
        void setSize(float x, float y);
        void setSize(pix_or_norm x, pix_or_norm y);
        void setSize(Vec2D<pix_or_norm> v);

        auto getTransform() -> Transform;
        void setTransform(Transform newTransform);

        auto getPositionProperties() -> Transform::Properties;
        auto getSizeProperties() -> Transform::Properties;
        auto setPositionProperties(Transform::Properties newProps);
        auto setSizeProperties(Transform::Properties newProps);

    private:
        Transform localTransform;
    };

#include "trc/ui/CRTPNode.inl"

} // namespace trc::ui
