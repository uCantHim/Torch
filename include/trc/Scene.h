#pragma once

#include <vkb/Buffer.h>

#include "Boilerplate.h"
#include "base/SceneBase.h"
#include "base/SceneRegisterable.h"
#include "Node.h"
#include "Light.h"

namespace trc
{
    class Pickable;

    class Scene : public SceneBase
    {
    public:
        Scene();

        auto getRoot() noexcept -> Node&;
        auto getRoot() const noexcept -> const Node&;

        void updateTransforms();

        void add(SceneRegisterable& object);
        void addLight(const Light& light);
        void removeLight(const Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getLightRegistry() noexcept -> LightRegistry&;
        auto getPickingBuffer() const noexcept -> vk::Buffer;
        auto getPickedObject() -> std::optional<Pickable*>;

    private:
        Node root;

        LightRegistry lightRegistry;

        void updatePicking();
        vkb::Buffer pickingBuffer;
        ui32 currentlyPicked{ 0 };
    };


    class SceneDescriptor
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        static void setActiveScene(const Scene& scene) noexcept;

    private:
        static void vulkanStaticInit();
        static void vulkanStaticDestroy();
        static inline vkb::StaticInit _init{
            vulkanStaticInit, vulkanStaticDestroy
        };

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vk::UniqueDescriptorSet descSet;
        static inline DescriptorProvider provider{ {}, {} };
    };
} // namespace trc
