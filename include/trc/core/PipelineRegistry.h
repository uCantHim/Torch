#pragma once

#include <mutex>
#include <optional>
#include <source_location>
#include <variant>
#include <vector>

#include "trc/core/DescriptorRegistry.h"
#include "trc/core/Instance.h"
#include "trc/core/Pipeline.h"
#include "trc/core/PipelineLayoutTemplate.h"
#include "trc/core/PipelineTemplate.h"
#include "trc/core/RenderPassRegistry.h"
#include "trc_util/data/SafeVector.h"

namespace trc
{
    class PipelineStorage;
    class ResourceConfig;

    /**
     * @brief Error thrown by `PipelineRegistry` if an operation is requested
     *        for the wrong type of pipeline.
     */
    class InvalidPipelineType : Exception
    {
    public:
        InvalidPipelineType(std::source_location loc,
                            const std::string& whatHappened,
                            vk::PipelineBindPoint expected,
                            vk::PipelineBindPoint actual);
    };

    /**
     * @brief
     */
    class PipelineRegistry
    {
    public:
        /**
         * @brief Register a template for a pipeline layout
         */
        static auto registerPipelineLayout(PipelineLayoutTemplate _template) -> PipelineLayout::ID;

        /**
         * @brief Clone a pipeline layout template as a basis for a new one
         */
        static auto clonePipelineLayout(PipelineLayout::ID id) -> PipelineLayoutTemplate;

        /**
         * @brief Register a template for a graphics pipeline
         *
         * One must provide additional render pass compatibility information
         * when creating a graphics pipeline.
         *
         * This can be a render pass object for traditional render pass-based
         * rendering via the `RenderPassInfo` struct. Alternatively, for use
         * with `VK_KHR_dynamic_rendering`, we can omit the render pass object
         * and instead provide attachment information via a
         * `DynamicRenderingInfo` struct.
         *
         * As a third option, we can specify either of the two previous options
         * via a reference name. That name will be used during creation of the
         * pipeline object (may be lazy) to query the respective information
         * from the `RenderPassRegistry` used at that time.
         */
        static auto registerPipeline(PipelineTemplate pipelineTemplate,
                                     PipelineLayout::ID layout,
                                     const RenderPassDefinition& renderPass)
            -> Pipeline::ID;

        /**
         * @brief Register a template for a compute pipeline
         */
        static auto registerPipeline(ComputePipelineTemplate pipelineTemplate,
                                     PipelineLayout::ID layout)
            -> Pipeline::ID;

        /**
         * @brief Clone a graphics pipeline template as a basis for a new one
         *
         * @param Pipeline::ID id The pipeline to clone
         *
         * @return std::pair<PipelineTemplate, RenderPassDefinition>
         * @throw InvalidPipelineType if `id` does not refer to a graphics
         *        pipeline.
         */
        static auto cloneGraphicsPipeline(Pipeline::ID id)
            -> std::pair<PipelineTemplate, RenderPassDefinition>;

        /**
         * @brief Clone a compute pipeline template as a basis for a new one
         *
         * @throw InvalidPipelineType if `id` does not refer to a compute
         *        pipeline.
         */
        static auto cloneComputePipeline(Pipeline::ID id) -> ComputePipelineTemplate;

        /**
         * @brief Query a pipeline's layout
         */
        static auto getPipelineLayout(Pipeline::ID id) -> PipelineLayout::ID;

        /**
         * The returned compatibility information may be either a reference to
         * a render pass registered at a `RenderPassRegistry`, or a concrete
         * object with that information. You can resolve the result value to a
         * concrete object with the `resolveRenderPass` helper function.
         *
         * @return Render pass compatibility information about the pipeline,
         *         *if* it is a graphics pipeline. Otherwise nothing.
         */
        static auto getPipelineRenderPass(Pipeline::ID id)
            -> std::optional<RenderPassDefinition>;

        /**
         * @brief Create a pipeline storage object
         */
        static auto makeStorage(const Instance& instance, ResourceConfig& resourceConfig)
            -> u_ptr<PipelineStorage>;

        /**
         * @brief Creates pipeline objects from a stored template
         */
        class PipelineFactory
        {
        public:
            PipelineFactory() = default;
            PipelineFactory(PipelineTemplate t, PipelineLayout::ID layout, RenderPassName rp);
            PipelineFactory(PipelineTemplate t, PipelineLayout::ID layout, RenderPassCompatInfo r);
            PipelineFactory(ComputePipelineTemplate t, PipelineLayout::ID layout);

            struct GraphicsPipelineInfo
            {
                PipelineTemplate tmpl;
                std::variant<RenderPassName, RenderPassCompatInfo> renderPassCompatInfo;
            };

            auto getLayout() const -> PipelineLayout::ID;

            /**
             * @return nullptr if the factory has no render pass compatibility
             *         information. This can only happen if the factory produces
             *         a compute pipeline.
             */
            auto getRenderPassCompatInfo() const
                -> const std::variant<RenderPassName, RenderPassCompatInfo>*;

            /**
             * @brief Invoke the factory to create its pipeline
             */
            auto create(const Instance& instance,
                        ResourceConfig& resourceConfig,
                        PipelineLayout& layout)
                -> Pipeline;

            auto clone() const -> std::variant<GraphicsPipelineInfo, ComputePipelineTemplate>;

        private:
            static auto create(PipelineTemplate& p,
                               const RenderPassInfo& compatInfo,
                               const Instance& instance,
                               PipelineLayout& layout) -> Pipeline;

            static auto create(PipelineTemplate& p,
                               const DynamicRenderingInfo& compatInfo,
                               const Instance& instance,
                               PipelineLayout& layout) -> Pipeline;

            static auto create(ComputePipelineTemplate& p,
                               const Instance& instance,
                               PipelineLayout& layout) -> Pipeline;

            PipelineLayout::ID layoutId;
            std::variant<GraphicsPipelineInfo, ComputePipelineTemplate> _template;
        };

        /**
         * @brief Creates pipeline layouts from a stored template
         */
        class LayoutFactory
        {
        public:
            LayoutFactory() = default;
            explicit LayoutFactory(PipelineLayoutTemplate t);

            auto create(const Instance& instance, ResourceConfig& resourceConfig) -> PipelineLayout;
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
                                       ResourceConfig& resourceConfig,
                                       PipelineLayout& layout)
                -> Pipeline;

            auto invokeLayoutFactory(PipelineLayout::ID id,
                                     const Instance& instance,
                                     ResourceConfig& resourceConfig)
                -> PipelineLayout;

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
     * @brief A container for concrete instances of pipelines
     *
     * Pipeline templates registered at the pipeline registry are instantiated
     * and stored by `PipelineStorage` objects.
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
                        ResourceConfig& resourceConfig);

        auto createPipeline(FactoryType& factory) -> u_ptr<Pipeline>;

        typename PipelineRegistry::StorageAccessInterface registry;
        const Instance& instance;
        ResourceConfig* resourceConfig;

        util::SafeVector<PipelineLayout, 20> layouts;
        util::SafeVector<Pipeline, 20> pipelines;
    };
} // namespace trc
