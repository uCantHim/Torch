#include "trc/ShadowRegistry.h"

#include "trc/base/Logging.h"



namespace trc
{

ShadowRegistry::ShadowRegistry()
    :
    createDispatcher(EventDispatcher<ShadowCreateEvent>::make()),
    destroyDispatcher(EventDispatcher<ShadowDestroyEvent>::make())
{}

auto ShadowRegistry::makeShadow(const ShadowCreateInfo& createInfo) -> ShadowID
{
    const ShadowID id{ shadowIdPool.generate() };
    shadows.try_emplace(id, createInfo);
    createDispatcher->notify({ id, createInfo });

    return id;
}

void ShadowRegistry::freeShadow(ShadowID id)
{
    try {
        destroyDispatcher->notify({ id });
    } catch (const std::exception& err) {
        log::error << log::here() << ": Event listener threw an exception: " << err.what();
    }

    shadows.erase(id);
    shadowIdPool.free(ui32{id});
}

} // namespace trc
