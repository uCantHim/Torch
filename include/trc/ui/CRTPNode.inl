#include "trc/ui/CRTPNode.h"



template<typename Derived>
inline void trc::ui::CRTPNode<Derived>::attach(Derived& child)
{
    children.emplace_back(&child);
    child.parent = static_cast<Derived*>(this);
}

template<typename Derived>
inline void trc::ui::CRTPNode<Derived>::detach(Derived& child)
{
    {
        auto range = children.iter();
        for (auto it = range.begin(); it != range.end(); it++)
        {
            if (*it == &child)
            {
                (*it)->parent = nullptr;
                children.erase(it);
            }
        }
    }
    children.update();
}

template<typename Derived>
template<std::invocable<Derived&> F>
inline void trc::ui::CRTPNode<Derived>::foreachChild(F func)
{
    for (auto child : children.iter()) {
        func(*child);
    }
    children.update();
}



template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getPos() const -> vec2
{
    return localTransform.position;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getSize() const -> vec2
{
    return localTransform.size;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPos(vec2 newPos)
{
    localTransform.position = newPos;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPos(float x, float y)
{
    localTransform.position = { x, y };
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPos(pix_or_norm x, pix_or_norm y)
{
    localTransform.position = { x.value, y.value };
    localTransform.positionFormat = { x.format, y.format };
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPos(Vec2D<pix_or_norm> v)
{
    setPos(v.x, v.y);
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSize(vec2 newSize)
{
    localTransform.size = newSize;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSize(float x, float y)
{
    localTransform.size = { x, y };
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSize(pix_or_norm x, pix_or_norm y)
{
    localTransform.size = { x.value, y.value };
    localTransform.sizeFormat = { x.format, y.format };
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSize(Vec2D<pix_or_norm> v)
{
    setSize(v.x, v.y);
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getTransform() -> Transform
{
    return localTransform;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setTransform(Transform newTransform)
{
    localTransform = newTransform;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getPositionFormat() const -> Vec2D<Format>
{
    return localTransform.positionFormat;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getSizeFormat() const -> Vec2D<Format>
{
    return localTransform.sizeFormat;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getPositionScaling() const -> Vec2D<Scale>
{
    return localTransform.positionScaling;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getSizeScaling() const -> Vec2D<Scale>
{
    return localTransform.sizeScaling;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPositionFormat(Vec2D<Format> newProps)
{
    localTransform.positionFormat = newProps;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSizeFormat(Vec2D<Format> newFormat)
{
    localTransform.sizeFormat = newFormat;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPositionScaling(Vec2D<Scale> newScaling)
{
    localTransform.positionScaling = newScaling;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSizeScaling(Vec2D<Scale> newScaling)
{
    localTransform.sizeScaling = newScaling;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPositioning(
    Vec2D<Format> newFormat,
    Vec2D<Scale> newScaling)
{
    localTransform.positionFormat = newFormat;
    localTransform.positionScaling = newScaling;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSizing(
    Vec2D<Format> newFormat,
    Vec2D<Scale> newScaling)
{
    localTransform.sizeFormat = newFormat;
    localTransform.sizeScaling = newScaling;
}
