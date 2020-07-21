#include "Node.h"

#include <algorithm>



/**
 * Node uses the temporary matrix mechanism of Transformation as a storage
 * for its global transformation.
 */

trc::Node::Node(Node&& other) noexcept
    :
    parent(other.parent),
    children(std::move(other.children))
{
    parent->detach(other);
    parent->attach(*this);
}

trc::Node::~Node()
{
    detachFromParent();
    for (Node* child : children)
    {
        child->parent = nullptr;
    }
}

auto trc::Node::operator=(Node&& rhs) noexcept -> Node&
{
    parent = rhs.parent;
    parent->detach(rhs);
    parent->attach(*this);

    children = std::move(rhs.children);

    return *this;
}

auto trc::Node::getGlobalTransform() const noexcept -> const mat4&
{
    if (parent != nullptr) {
        return globalTransform;
    }

    return getTransformationMatrix();
}

void trc::Node::update() noexcept
{
    globalTransform = getTransformationMatrix();
    for (Node* child : children)
    {
        child->update(globalTransform);
    }
}

void trc::Node::update(const mat4& parentTransform) noexcept
{
    globalTransform = parentTransform * getTransformationMatrix();
    for (Node* child : children)
    {
        child->update(globalTransform);
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
