#pragma once

#include <vector>
#include <functional>

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
        static inline std::vector<std::function<void()>> recreateFunctions;
    };
} // namespace trc
