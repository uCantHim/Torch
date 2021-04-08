#include "CRTPNode.h"



template<typename Derived>
trc::ui::CRTPNode<Derived>::~CRTPNode<Derived>()
{
    if (parent != nullptr) {
        parent->detach(static_cast<Derived&>(*this));
    }
}

template<typename Derived>
inline void trc::ui::CRTPNode<Derived>::attach(Derived& child)
{
    children.push_back(&child);
    child.parent = static_cast<Derived*>(this);
}

template<typename Derived>
inline void trc::ui::CRTPNode<Derived>::detach(Derived& child)
{
    for (auto it = children.begin(); it != children.end(); it++)
    {
        if (*it == &child)
        {
            (*it)->parent = nullptr;
            it = --children.erase(it);
        }
    }
}

template<typename Derived>
inline void trc::ui::CRTPNode<Derived>::clearChildren()
{
    for (auto child : children) {
        child->parent = nullptr;
    }
    children.clear();
}

template<typename Derived>
template<std::invocable<Derived&> F>
inline void trc::ui::CRTPNode<Derived>::foreachChild(F func)
{
    for (auto child : children) {
        func(*child);
    }
}



template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getPos() -> vec2
{
    return localTransform.position;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getSize() -> vec2
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
inline void trc::ui::TransformNode<Derived>::setPos(_pix x, _pix y)
{
    localTransform.position = { x.value, y.value };
    localTransform.posProp.format = Format::ePixel;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setPos(_norm x, _norm y)
{
    localTransform.position = { x.value, y.value };
    localTransform.posProp.format = Format::eNorm;
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
inline void trc::ui::TransformNode<Derived>::setSize(_pix x, _pix y)
{
    localTransform.size = { x.value, y.value };
    localTransform.sizeProp.format = Format::ePixel;
}

template<typename Derived>
inline void trc::ui::TransformNode<Derived>::setSize(_norm x, _norm y)
{
    localTransform.size = { x.value, y.value };
    localTransform.posProp.format = Format::eNorm;
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
inline auto trc::ui::TransformNode<Derived>::getPositionProperties() -> Transform::Properties
{
    return localTransform.posProp;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::getSizeProperties() -> Transform::Properties
{
    return localTransform.sizeProp;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::setPositionProperties(Transform::Properties newProps)
{
    localTransform.posProp = newProps;
}

template<typename Derived>
inline auto trc::ui::TransformNode<Derived>::setSizeProperties(Transform::Properties newProps)
{
    localTransform.sizeProp = newProps;
}
