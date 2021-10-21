#include "RenderConfiguration.h"



trc::RenderConfig::RenderConfig(RenderLayout layout)
    :
    layout(std::move(layout))
{
}

auto trc::RenderConfig::getLayout() -> RenderLayout&
{
    return layout;
}
