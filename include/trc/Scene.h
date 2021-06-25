#pragma once

#include <vkb/Buffer.h>

#include "Types.h"
#include "core/SceneBase.h"
#include "Node.h"
#include "LightRegistry.h"
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

        void addLight(Light& light);
        void removeLight(const Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getLightRegistry() noexcept -> LightRegistry&;
        auto getLightRegistry() const noexcept -> const LightRegistry&;

        /**
         * @brief Get additional per-scene render passes for a render stage
         *
         * Scenes can declare additional render passes for certain render
         * stages that are not executed renderer-wide but only for this
         * scene. The render stage has to be enabled in the renderer.
         *
         * The renderer calls this function for every enabled render stage
         * in its render graph.
         *
         * TODO: Currently only provides shadow passes. Implement user-
         * defined render passes.
         */
        auto getLocalRenderPasses(RenderStageType::ID stageType) -> std::vector<RenderPass::ID>;

        /**
         * @return const SceneDescriptor& The scene's descriptor
         */
        auto getDescriptor() const noexcept -> const SceneDescriptor&;

        auto getPickedObject() -> std::optional<Pickable*>;

    private:
        Node root;

        LightRegistry lightRegistry;

        // Must be initialized after the light registry
        SceneDescriptor descriptor;

        void updatePicking();
        ui32 currentlyPicked{ 0 };
    };
} // namespace trc
