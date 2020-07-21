#pragma once

#include <memory>
#include <vector>

#include "utils/Transformation.h"

namespace trc
{
    class Node : public trc::Transformation
    {
    public:
        Node() = default;
        Node(const Node& other) = delete;
        Node(Node&& other) noexcept;
        ~Node();

        auto operator=(const Node& rhs) -> Node& = delete;
        auto operator=(Node&& rhs) noexcept -> Node&;

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
        mat4 globalTransform{ 1.0f };

        Node* parent{ nullptr };
        std::vector<Node*> children;
    };
} // namespace trc
