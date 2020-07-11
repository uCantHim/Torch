#pragma once

#include <memory>
#include <vector>

#include "utils/Transformation.h"

class Node : public trc::Transformation
{
public:
    auto getGlobalTransform() const noexcept -> const mat4&;

    void update() noexcept;
    void update(const mat4& parentTransform) noexcept;
    /**
     * @brief Updates children without a parent transformation
     *
     * This saves a matrix multiplication per child.
     */
    void updateAsRoot() noexcept;

    void attach(Node& child);
    void detach(Node& child);
    void detachFromParent();

private:
    Node* parent{ nullptr };
    std::vector<Node*> children;
};
