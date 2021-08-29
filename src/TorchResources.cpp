#include "TorchResources.h"



namespace
{
    using namespace trc;

    constexpr ui32 DEFERRED_SUBPASSES{ 3 };
    constexpr ui32 SHADOW_SUBPASSES{ 1 };

    static RenderStageType::ID deferred{
        RenderStageType::createAtNextIndex(DEFERRED_SUBPASSES).first
    };
    static RenderStageType::ID shadow{
        RenderStageType::createAtNextIndex(SHADOW_SUBPASSES).first
    };

} // anonymous namespace

auto trc::RenderStageTypes::getDeferred() -> RenderStageType::ID
{
    return deferred;
}

auto trc::RenderStageTypes::getShadow() -> RenderStageType::ID
{
    return shadow;
}
