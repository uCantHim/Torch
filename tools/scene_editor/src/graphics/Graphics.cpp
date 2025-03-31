#include "graphics/Graphics.h"

#include <trc/Torch.h>



GraphicsStack::GraphicsStack()
    :
    instance([]{
        trc::init();
        return trc::Instance{};
    }())
{
}

GraphicsStack::~GraphicsStack() noexcept
{
    trc::terminate();
}

auto GraphicsStack::getDevice() -> trc::Device&
{
    return instance.getDevice();
}

auto GraphicsStack::getInstance() -> trc::Instance&
{
    return instance;
}
