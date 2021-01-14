#include "TorchResources.h"

#include <memory>
#include <iostream>



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

    static vkb::QueueFamilyIndex mainRenderQueueFamily;
    static std::unique_ptr<vkb::ExclusiveQueue> mainRenderQueue;

    static vkb::QueueFamilyIndex mainPresentQueueFamily;
    static std::unique_ptr<vkb::ExclusiveQueue> mainPresentQueue;

    static std::vector<std::unique_ptr<vkb::ExclusiveQueue>> transferQueues;
} // anonymous namespace

auto trc::RenderStageTypes::getDeferred() -> RenderStageType::ID
{
    return deferred;
}

auto trc::RenderStageTypes::getShadow() -> RenderStageType::ID
{
    return shadow;
}



void trc::Queues::init(vkb::QueueManager& queueManager)
{
    mainRenderQueue = std::make_unique<vkb::ExclusiveQueue>(
        queueManager.reservePrimaryQueue(vkb::QueueType::graphics)
    );
    mainRenderQueueFamily = queueManager.getPrimaryQueueFamily(vkb::QueueType::graphics);

    auto [queue, family] = queueManager.getAnyQueue(vkb::QueueType::presentation);
    mainPresentQueue = std::make_unique<vkb::ExclusiveQueue>(queueManager.reserveQueue(queue));
    mainPresentQueueFamily = family;

    ui32 totalTransfer{ 0 };
    try {
        // Try to retrieve at least four transfer queues
        constexpr ui32 maxTransfer{ 4 };
        ui32 primaryTransferCount = glm::min(
            queueManager.getPrimaryQueueCount(vkb::QueueType::transfer),
            maxTransfer
        );
        // Primary transfer queues first
        for (ui32 i = 0; i < primaryTransferCount; i++, totalTransfer++)
        {
            auto queue = queueManager.reservePrimaryQueue(vkb::QueueType::transfer);
            transferQueues.emplace_back(new vkb::ExclusiveQueue(queue));
        }
        // Fill with additional non-primary transfer queues
        for (; totalTransfer < maxTransfer; totalTransfer++)
        {
            auto [queue, family] = queueManager.getAnyQueue(vkb::QueueType::transfer);
            transferQueues.emplace_back(new vkb::ExclusiveQueue(queueManager.reserveQueue(queue)));
        }
    }
    catch (const vkb::QueueReservedError&) {}

    std::cout << "--- Using " << totalTransfer << " transfer queues\n";
}

auto trc::Queues::getMainRender() -> vkb::ExclusiveQueue&
{
    return *mainRenderQueue;
}

auto trc::Queues::getMainRenderFamily() -> vkb::QueueFamilyIndex
{
    return mainRenderQueueFamily;
}

auto trc::Queues::getMainPresent() -> vkb::ExclusiveQueue&
{
    return *mainPresentQueue;
}

auto trc::Queues::getMainPresentFamily() -> vkb::QueueFamilyIndex
{
    return mainPresentQueueFamily;
}

auto trc::Queues::getTransfer() -> vkb::ExclusiveQueue&
{
    static ui32 nextQueue{ 0 };

    nextQueue = (nextQueue + 1) % transferQueues.size();
    return *transferQueues[nextQueue];
}
