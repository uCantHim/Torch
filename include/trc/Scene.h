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

    private:
        Node root;
    };

    void updateScene(Scene& scene) noexcept {
        scene.getRoot().updateAsRoot();
    }
} // namespace trc
