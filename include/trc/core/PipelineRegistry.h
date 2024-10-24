#pragma once

#include <vector>
#include <functional>
#include <variant>
#include <mutex>

#include "trc/core/Instance.h"
#include "trc/core/Pipeline.h"
#include "trc/core/PipelineLayoutTemplate.h"
#include "trc/core/PipelineTemplate.h"
#include "trc/core/RenderConfiguration.h"
#include "trc_util/data/SafeVector.h"

namespace trc
{
    class PipelineStorage;
    class PipelineRegistry;

    auto registerPipeline(PipelineTemplate t, PipelineLayout::ID layout, RenderPassName renderPass)
        -> Pipeline::ID;

    auto registerPipeline(ComputePipelineTemplate t, PipelineLayout::ID layout)
        -> Pipeline::ID;

    /**
     * @brief
     */
    class PipelineRegistry
    {
    public:
        static auto registerPipelineLayout(PipelineLayoutTemplate _template) -> PipelineLayout::ID;
        static auto clonePipelineLayout(PipelineLayout::ID id) -> PipelineLayoutTemplate;

        static auto registerPipeline(PipelineTemplate pipelineTemplate,
                                     PipelineLayout::ID layout,
                                     RenderPassName renderPass)
            -> Pipeline::ID;
        static auto registerPipeline(ComputePipelineTemplate pipelineTemplate,
                                     PipelineLayout::ID layout)
            -> Pipeline::ID;

        static auto cloneGraphicsPipeline(Pipeline::ID id) -> PipelineTemplate;
        static auto cloneComputePipeline(Pipeline::ID id) -> ComputePipelineTemplate;
        static auto getPipelineLayout(Pipeline::ID id) -> PipelineLayout::ID;
        static auto getPipelineRenderPass(Pipeline::ID id) -> RenderPassName;

        /**
         * @brief Create a pipeline storage object
         */
        static auto makeStorage(const Instance& instance, RenderConfig& renderConfig)
            -> u_ptr<PipelineStorage>;

        /**
         * @brief Creates pipeline objects from a stored template
         */
        class PipelineFactory
        {
        public:
            PipelineFactory() = default;
            PipelineFactory(PipelineTemplate t, PipelineLayout::ID layout, RenderPassName rp);
            PipelineFactory(ComputePipelineTemplate t, PipelineLayout::ID layout);

            auto getLayout() const -> PipelineLayout::ID;
            auto getRenderPassName() const -> const RenderPassName&;

            auto create(const Instance& instance, RenderConfig& renderConfig, PipelineLayout& layout)
                -> Pipeline;
            auto clone() const -> std::variant<PipelineTemplate, ComputePipelineTemplate>;

        private:
            auto create(PipelineTemplate& p,
                        const Instance& instance,
                        RenderConfig& renderConfig,
                        PipelineLayout& layout) const -> Pipeline;
            auto create(ComputePipelineTemplate& p,
                        const Instance& instance,
                        RenderConfig& renderConfig,
                        PipelineLayout& layout) const -> Pipeline;

            PipelineLayout::ID layoutId;
            RenderPassName renderPassName;
            std::variant<PipelineTemplate, ComputePipelineTemplate> _template;
        };

        /**
         * @brief Creates pipeline layouts from a stored template
         */
        class LayoutFactory
        {
        public:
            LayoutFactory() = default;
            explicit LayoutFactory(PipelineLayoutTemplate t);

            auto create(const Instance& instance, RenderConfig& renderConfig) -> PipelineLayout;
            auto clone() const -> PipelineLayoutTemplate;

        private:
            PipelineLayoutTemplate _template;
        };

        /**
         * @brief Used internally for communication with PipelineStorage
         */
        class StorageAccessInterface
        {
        public:
            auto getPipelineLayout(Pipeline::ID id) -> PipelineLayout::ID;

            auto invokePipelineFactory(Pipeline::ID id,
                                       const Instance& instance,
                                       RenderConfig& renderConfig,
                                       PipelineLayout& layout)
                -> Pipeline;

            auto invokeLayoutFactory(PipelineLayout::ID id,
                                     const Instance& instance,
                                     RenderConfig& renderConfig)
                -> PipelineLayout;

            template<std::invocable<PipelineFactory&> F>
            static void foreachFactory(F&& func)
            {
                std::scoped_lock lock(factoryLock);
                for (auto& factory : factories) {
                    func(factory);
                }
            }

        private:
            friend PipelineRegistry;
            StorageAccessInterface() = default;
        };

    private:
        static inline auto _allocPipelineLayoutId() -> PipelineLayout::ID;
        static inline auto _allocPipelineId() -> Pipeline::ID;
        static inline auto _registerPipelineFactory(PipelineFactory factory) -> Pipeline::ID;

        static inline data::IdPool<ui64> pipelineLayoutIdPool;
        static inline data::IdPool<ui64> pipelineIdPool;

        static inline std::mutex layoutFactoryLock;
        static inline std::vector<LayoutFactory> layoutFactories;
        static inline std::mutex factoryLock;
        static inline std::vector<PipelineFactory> factories;
    };

    /**
     * @brief
     */
    class PipelineStorage
    {
    public:
        PipelineStorage(const PipelineStorage&) = delete;
        PipelineStorage(PipelineStorage&&) noexcept = delete;
        PipelineStorage& operator=(const PipelineStorage&) = delete;
        PipelineStorage& operator=(PipelineStorage&&) noexcept = delete;

        ~PipelineStorage() noexcept = default;

        auto get(Pipeline::ID pipeline) -> Pipeline&;
        auto getLayout(PipelineLayout::ID id) -> PipelineLayout&;

        /**
         * @brief Destroy all pipelines and pipeline layouts
         */
        void clear();

    private:
        friend PipelineRegistry;
        using FactoryType = typename PipelineRegistry::PipelineFactory;

        PipelineStorage(typename PipelineRegistry::StorageAccessInterface interface,
                        const Instance& instance,
                        RenderConfig& renderConfig);

        auto createPipeline(FactoryType& factory) -> u_ptr<Pipeline>;

        typename PipelineRegistry::StorageAccessInterface registry;
        const Instance& instance;
        RenderConfig* renderConfig;

        util::SafeVector<PipelineLayout, 20> layouts;
        util::SafeVector<Pipeline, 20> pipelines;
    };
} // namespace trc
