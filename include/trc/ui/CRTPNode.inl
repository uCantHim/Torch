#include "CRTPNode.h"



template<typename Derived>
trc::ui::CRTPNode<Derived>::~CRTPNode<Derived>()
{
    if (parent != nullptr) {
        parent->detach(static_cast<Derived&>(*this));
    }
}

template<typename Derived>
void trc::ui::CRTPNode<Derived>::attach(Derived& child)
{
    children.push_back(&child);
    child.parent = static_cast<Derived*>(this);
}

template<typename Derived>
void trc::ui::CRTPNode<Derived>::detach(Derived& child)
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
void trc::ui::CRTPNode<Derived>::clearChildren()
{
    for (auto child : children) {
        child->parent = nullptr;
    }
    children.clear();
}

template<typename Derived>
template<std::invocable<Derived&> F>
void trc::ui::CRTPNode<Derived>::foreachChild(F func)
{
    for (auto child : children) {
        func(*child);
    }
}



template<typename Derived>
auto trc::ui::TransformNode<Derived>::getPos() -> vec2
{
    return localTransform.position;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::getSize() -> vec2
{
    return localTransform.size;
}

template<typename Derived>
void trc::ui::TransformNode<Derived>::setPos(vec2 newPos)
{
    localTransform.position = newPos;
}

template<typename Derived>
void trc::ui::TransformNode<Derived>::setSize(vec2 newSize)
{
    localTransform.size = newSize;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::getTransform() -> Transform
{
    return localTransform;
}

template<typename Derived>
void trc::ui::TransformNode<Derived>::setTransform(Transform newTransform)
{
    localTransform = newTransform;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::getPositionProperties() -> Transform::Properties
{
    return localTransform.posProp;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::getSizeProperties() -> Transform::Properties
{
    return localTransform.sizeProp;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::setPositionProperties(Transform::Properties newProps)
{
    localTransform.posProp = newProps;
}

template<typename Derived>
auto trc::ui::TransformNode<Derived>::setSizeProperties(Transform::Properties newProps)
{
    localTransform.sizeProp = newProps;
}
