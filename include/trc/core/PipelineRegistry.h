#pragma once

#include <vector>
#include <functional>
#include <ranges>
#include <mutex>

#include <nc/data/ObjectId.h>

#include "Instance.h"
#include "Pipeline.h"
//#include "RenderConfiguration.h"

namespace trc
{
    template<typename T, typename U>
    concept Range = requires (T range) {
        std::ranges::range<T>;
        std::is_same_v<U, std::decay_t<decltype(std::begin(range))>>;
    };

    template<typename T, typename RenderConf>
    concept PipelineFactoryFunc = requires (T func) {
        std::invocable<T, const Instance&, const RenderConf&>;
        std::is_same_v<Pipeline, std::invoke_result_t<T, const Instance, const RenderConf>>;
    };

    template<typename T>
    class PipelineRegistry;

    /**
     * @brief
     */
    template<typename T>
    class PipelineStorage
    {
    private:
        friend class PipelineRegistry<T>;

        /**
         * @brief
         *
         * @param T& renderConfig
         */
        explicit PipelineStorage(const Instance& instance, T& renderConfig);

    public:
        auto get(Pipeline::ID pipeline) -> Pipeline&;

        void recreateAll();

    private:
        const Instance& instance;
        T* renderConfig;
        std::vector<Pipeline> pipelines;
    };

    /**
     * @brief
     */
    template<typename T>
    class PipelineRegistry
    {
    public:
        using FactoryFunc = std::function<Pipeline(const Instance&, const T&)>;

        /**
         * @brief Create a pipeline storage object
         */
        static auto createStorage(const Instance& instance, T& renderConfig) -> PipelineStorage<T>
        {
            return PipelineStorage<T>(instance, renderConfig);
        }

        template<PipelineFactoryFunc<T> F>
        static auto registerPipeline(F&& factoryFunc) -> Pipeline::ID;

        template<std::invocable<FactoryFunc> F>
        static void foreachFactory(F&& func)
        {
            std::lock_guard lock(factoryLock);
            for (auto& factory : factories) {
                func(factory);
            }
        }

    private:
        static inline std::mutex factoryLock;

        static inline data::IdPool idPool;
        static inline std::vector<FactoryFunc> factories;
    };



    template<typename T>
    PipelineStorage<T>::PipelineStorage(const Instance& instance, T& renderConfig)
        :
        instance(instance),
        renderConfig(&renderConfig)
    {
        recreateAll();
    }

    template<typename T>
    auto PipelineStorage<T>::get(Pipeline::ID pipeline) -> Pipeline&
    {
        return pipelines.at(pipeline);
    }

    template<typename T>
    void PipelineStorage<T>::recreateAll()
    {
        pipelines.clear();
        PipelineRegistry<T>::foreachFactory([this](auto factory) {
            pipelines.push_back(factory(instance, *renderConfig));
        });
    }



    template<typename T>
    template<PipelineFactoryFunc<T> F>
    auto PipelineRegistry<T>::registerPipeline(F&& factoryFunc) -> Pipeline::ID
    {
        const Pipeline::ID id{ idPool.generate() };

        std::lock_guard lock(factoryLock);
        if (factories.size() <= id) {
            factories.resize(id);
        }
        factories.emplace(factories.begin() + id, std::move(factoryFunc));

        return id;
    }
} // namespace trc
