#pragma once

#include <vkb/Buffer.h>

#include "Boilerplate.h"
#include "base/SceneBase.h"
#include "base/SceneRegisterable.h"
#include "Node.h"
#include "Light.h"

namespace trc
{
    class Scene : public SceneBase
    {
    public:
        Scene();

        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        void updateTransforms();

        void add(SceneRegisterable& object);
        void addLight(Light& light);
        void removeLight(Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;

    private:
        Node root;

        void updateLightBuffer();
        std::vector<Light*> lights;
        vkb::Buffer lightBuffer;
    };
} // namespace trc
