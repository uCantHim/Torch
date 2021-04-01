#pragma once

#include <vector>
#include <functional>
#include <ranges>

#include "Pipeline.h"

namespace trc
{
    template<typename T, typename U>
    concept Range = requires (T range) {
        std::ranges::range<T>;
        std::is_same_v<U, std::decay_t<decltype(std::begin(range))>>;
    };

    template<typename T>
    concept PipelineMultiCreateFunc = requires (T func) {
        { func() } -> Range<Pipeline>;
    };

    class PipelineRegistry
    {
    public:
        using PipelineCreateFunc = std::function<Pipeline()>;

        static auto registerPipeline(PipelineCreateFunc recreateFunction) -> Pipeline::ID
        {
            auto id = Pipeline::createAtNextIndex(recreateFunction()).first;
            recreateFunctions.emplace_back([=] {
                Pipeline::replace(id, recreateFunction());
            });

            return id;
        }

        template<PipelineMultiCreateFunc F>
        static auto registerPipeline(F recreateFunction) -> std::vector<Pipeline::ID>
        {
            std::vector<Pipeline::ID> ids;
            for (auto& pipeline : recreateFunction()) {
                ids.emplace_back(Pipeline::createAtNextIndex(std::move(pipeline)).first);
            }

            recreateFunctions.emplace_back([=] {
                for (size_t i = 0; auto& pipeline : recreateFunction())
                {
                    Pipeline::replace(ids.at(i), std::move(pipeline));
                    i++;
                }
            });

            return ids;
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
