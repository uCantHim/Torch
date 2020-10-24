#pragma once

#include <vkb/Buffer.h>

#include "Boilerplate.h"
#include "base/SceneBase.h"
#include "base/SceneRegisterable.h"
#include "Node.h"
#include "Light.h"
#include "SceneDescriptor.h"

namespace trc
{
    class Pickable;

    class Scene : public SceneBase
    {
    public:
        Scene();

        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        /**
         * @brief Update lights and picking
         *
         * Called automatically in Renderer::draw because it updates GPU
         * resources.
         */
        void update();

        /**
         * @brief Traverse the node tree and update the transform of each node
         *
         * This it NOT called automatically. You don't have to ever call
         * this if you don't want to, though you should if you want to
         * attach nodes to the scene. If you do, call this once per frame
         * (or more often if you want to, though that would be a heavy
         * waste of resources).
         *
         * You may also call this from another thread than the rendering
         * thread, though you have to take care because adding nodes while
         * traversing the tree is NOT thread safe and will probably crash
         * the program.
         */
        void updateTransforms();

        void addLight(const Light& light);
        void removeLight(const Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getLightRegistry() noexcept -> LightRegistry&;
        auto getLightRegistry() const noexcept -> const LightRegistry&;

        /**
         * @return const SceneDescriptor& The scene's descriptor
         */
        auto getDescriptor() const noexcept -> const SceneDescriptor&;

        auto getPickingBuffer() const noexcept -> vk::Buffer;
        auto getPickedObject() -> std::optional<Pickable*>;

    private:
        Node root;

        LightRegistry lightRegistry;

        void updatePicking();
        vkb::Buffer pickingBuffer;
        ui32 currentlyPicked{ 0 };

        // Must be initialized after the buffers
        SceneDescriptor descriptor;
    };
} // namespace trc
