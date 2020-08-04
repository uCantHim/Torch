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
        void addLight(Light& light);
        void removeLight(Light& light);

        auto getLightBuffer() const noexcept -> vk::Buffer;
        auto getPickingBuffer() const noexcept -> vk::Buffer;
        auto getPickedObject() -> std::optional<Pickable*>;

    private:
        Node root;

        void updateLightBuffer();
        std::vector<Light*> lights;
        vkb::Buffer lightBuffer;

        void updatePicking();
        vkb::Buffer pickingBuffer;
        ui32 currentlyPicked{ 0 };
    };


    class SceneDescriptor : public vkb::VulkanStaticInitialization<SceneDescriptor>
                          , public vkb::VulkanStaticDestruction<SceneDescriptor>
    {
    public:
        static auto getProvider() noexcept -> const DescriptorProviderInterface&;

        static void setActiveScene(const Scene& scene) noexcept;

    private:
        friend vkb::VulkanStaticInitialization<SceneDescriptor>;
        friend vkb::VulkanStaticDestruction<SceneDescriptor>;
        static inline vkb::VulkanStaticInitialization<SceneDescriptor> _force_init;
        static void vulkanStaticInit();
        static void vulkanStaticDestroy();

        static inline vk::UniqueDescriptorPool descPool;
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static inline vk::UniqueDescriptorSet descSet;
        static inline DescriptorProvider provider{ {}, {} };
    };
} // namespace trc
