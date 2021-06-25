#include "Node.h"

#include <algorithm>



trc::Node::Node(Node&& other) noexcept
    :
    Transformation(std::forward<Transformation>(other)),
    parent(other.parent),
    children(std::move(other.children))
{
    std::swap(globalTransformIndex, other.globalTransformIndex);
    if (parent != nullptr)
    {
        parent->detach(other);
        parent->attach(*this);
    }
    other.parent = nullptr;

    for (auto c : children) {
        c->parent = this;
    }
}

trc::Node::~Node()
{
    matrices.free(globalTransformIndex);
    detachFromParent();
    for (Node* child : children)
    {
        child->parent = nullptr;
    }
}

auto trc::Node::operator=(Node&& rhs) noexcept -> Node&
{
    Transformation::operator=(std::forward<Node>(rhs));

    std::swap(globalTransformIndex, rhs.globalTransformIndex);
    parent = rhs.parent;
    rhs.parent = nullptr;
    if (parent != nullptr)
    {
        parent->detach(rhs);
        parent->attach(*this);
    }

    children = std::move(rhs.children);
    for (auto c : children) {
        c->parent = this;
    }

    return *this;
}

auto trc::Node::getGlobalTransform() const noexcept -> const mat4&
{
    if (parent != nullptr) {
        return matrices.get(globalTransformIndex);
    }

    return getTransformationMatrix();
}

void trc::Node::update() noexcept
{
    matrices.set(globalTransformIndex, getTransformationMatrix());
    for (Node* child : children)
    {
        child->update(getGlobalTransform());
    }
}

void trc::Node::update(const mat4& parentTransform) noexcept
{
    matrices.set(globalTransformIndex, parentTransform * getTransformationMatrix());
    for (Node* child : children)
    {
        child->update(getGlobalTransform());
    }
}

void trc::Node::updateAsRoot() noexcept
{
    for (Node* child : children)
    {
        child->update();
    }
}

void trc::Node::attach(Node& child)
{
    child.detachFromParent();

    children.push_back(&child);
    child.parent = this;
    child.update(getGlobalTransform());
}

void trc::Node::detach(Node& child)
{
    children.erase(std::find(children.begin(), children.end(), &child));
    child.update();
}

void trc::Node::detachFromParent()
{
    if (parent != nullptr)
    {
        parent->detach(*this);
        parent = nullptr;
    }
}
