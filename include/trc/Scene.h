#pragma once

#include "base/SceneBase.h"
#include "Node.h"

namespace trc
{
    class Scene : public SceneBase
    {
    public:
        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        void updateTransformTree();

    private:
        Node root;
    };
} // namespace trc
