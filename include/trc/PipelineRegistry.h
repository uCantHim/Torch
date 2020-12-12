#pragma once

#include <vector>
#include <functional>

#include <vkb/VulkanBase.h>
#include <vkb/event/Event.h>

#include "Pipeline.h"

namespace trc
{
    class PipelineRegistry
    {
    public:
        static void registerPipeline(std::function<void()> recreateFunction)
        {
            recreateFunctions.push_back(std::move(recreateFunction));
        }

        static void recreateAll()
        {
            for (auto& f : recreateFunctions) {
                f();
            }
        }

    private:
        static inline vkb::StaticInit _init{
            [] {
                // Recreate all pipelines on swapchain recreation
                // TODO: This only works for a single swapchain
                vkb::on<vkb::SwapchainRecreateEvent>([](const auto&) { recreateAll(); });
            },
            [] {},
        };

        static inline std::vector<std::function<void()>> recreateFunctions;
    };
} // namespace trc
