#include "Node.h"

#include <algorithm>



/**
 * Node uses the temporary matrix mechanism of Transformation as a storage
 * for its global transformation.
 */

auto Node::getGlobalTransform() const noexcept -> const mat4&
{
    return getTransformationMatrix();
}

void Node::update() noexcept
{
    // Get global transform once because getTransformationMatrix() has an if statement
    const mat4& global = getTransformationMatrix();
    for (Node* child : children)
    {
        child->update(global);
    }
}

void Node::update(const mat4& parentTransform) noexcept
{
    setFromMatrixTemporary(getTransformationMatrix() * parentTransform);
    update();
}

void Node::updateAsRoot() noexcept
{
    for (Node* child : children)
    {
        child->update();
    }
}

void Node::attach(Node& child)
{
    child.detachFromParent();

    children.push_back(&child);
    child.parent = this;
}

void Node::detach(Node& child)
{
    children.erase(std::find(children.begin(), children.end(), &child));
}

void Node::detachFromParent()
{
    if (parent != nullptr)
    {
        parent->detach(*this);
        parent = nullptr;
    }
}
