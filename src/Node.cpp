#include "Node.h"

#include <algorithm>



trc::Node::Node()
{
    onLocalMatrixUpdate();
}

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

    detachFromParent();
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

auto trc::Node::getGlobalTransform() const noexcept -> mat4
{
    return globalTransformIndex.get();
}

auto trc::Node::getGlobalTransformID() const noexcept -> ID
{
    return globalTransformIndex;
}

void trc::Node::update() noexcept
{
    globalTransformIndex.set(getTransformationMatrix());
    for (Node* child : children)
    {
        child->update(getGlobalTransform());
    }
}

void trc::Node::update(const mat4& parentTransform) noexcept
{
    globalTransformIndex.set(parentTransform * getTransformationMatrix());
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
    if (parent == &child) {
        this->detachFromParent();
    }
    child.detachFromParent();

    children.push_back(&child);
    child.parent = this;
    child.update(getGlobalTransform());
}

void trc::Node::detach(Node& child)
{
    auto it = std::remove(children.begin(), children.end(), &child);
    if (it != children.end()) {
        children.erase(it);
    }
    child.parent = nullptr;
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

void trc::Node::onLocalMatrixUpdate()
{
    if (parent == nullptr) {
        globalTransformIndex.set(getTransformationMatrix());
    }
}
