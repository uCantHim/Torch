#pragma once

#include <unordered_map>

#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/util/EventDispatcher.h"

namespace trc
{
    class Camera;
    class ShadowRegistry;

    struct ShadowCreateInfo
    {
        uvec2 shadowMapResolution;
        s_ptr<Camera> camera;
    };

    namespace impl
    {
        struct ShadowIdTypeTag_ {};
        using ShadowID = data::TypesafeID<ShadowIdTypeTag_, ui32>;
    } // namespace impl

    using ShadowID = impl::ShadowID;

    /**
     * Stores descriptions of shadow maps.
     *
     * Device resource providers can subscribe to be notified when a shadow map
     * is created, so that they can allocate the corresponding device resources.
     */
    class ShadowRegistry
    {
    public:
        ShadowRegistry();

        auto makeShadow(const ShadowCreateInfo& createInfo) -> ShadowID;
        void freeShadow(ShadowID id);

    public:
        struct ShadowCreateEvent
        {
            ShadowID id;
            ShadowCreateInfo createInfo;
        };

        struct ShadowDestroyEvent
        {
            ShadowID id;
        };

        template<std::invocable<const ShadowCreateEvent&> F>
        auto onShadowCreate(F&& func) -> EventListener<ShadowCreateEvent>
        {
            // Notify the new listener of all existing shadow maps
            for (auto& [id, info] : shadows) {
                func(ShadowCreateEvent{ id, info });
            }
            return createDispatcher->registerListener(std::forward<F>(func));
        }

        template<std::invocable<const ShadowDestroyEvent&> F>
        auto onShadowDestroy(F&& func) -> EventListener<ShadowDestroyEvent> {
            return destroyDispatcher->registerListener(std::forward<F>(func));
        }

    private:
        s_ptr<EventDispatcher<ShadowCreateEvent>> createDispatcher;
        s_ptr<EventDispatcher<ShadowDestroyEvent>> destroyDispatcher;

        data::IdPool<ShadowID::IndexType> shadowIdPool;
        std::unordered_map<ShadowID, ShadowCreateInfo> shadows;
    };
} // namespace trc
