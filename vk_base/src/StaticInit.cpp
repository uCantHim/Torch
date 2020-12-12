#include "StaticInit.h"



vkb::StaticInit::StaticInit(std::function<void()> init)
{
    if (isInitialized) {
        init();
    }
    else {
        onInitCallbacks.push_back(std::move(init));
    }
}

vkb::StaticInit::StaticInit(std::function<void()> init, std::function<void()> destroy)
{
    if (isInitialized) {
        init();
    }
    else {
        onInitCallbacks.push_back(std::move(init));
    }

    onDestroyCallbacks.push_back(std::move(destroy));
}

void vkb::StaticInit::executeStaticInitializers()
{
    isInitialized = true;

    for (auto& func : onInitCallbacks) {
        std::invoke(func);
    }
    onInitCallbacks = {};
}

void vkb::StaticInit::executeStaticDestructors()
{
    for (auto& func : onDestroyCallbacks) {
        std::invoke(func);
    }
    onDestroyCallbacks = {};
}
