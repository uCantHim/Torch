#pragma once

#include <functional>
#include <shared_mutex>
#include <unordered_set>
#include <vector>

#include <trc_util/data/IndexMap.h>

#include "trc/Types.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderPass.h"
#include "trc/core/RenderStage.h"

namespace trc
{
    struct DrawEnvironment
    {
        RenderStage::ID currentRenderStage;
        RenderPass* currentRenderPass;
        SubPass::ID currentSubPass;
        Pipeline* currentPipeline;
    };

    using DrawableFunction = std::function<void(const DrawEnvironment&, vk::CommandBuffer)>;

    /**
     * @brief A basis of all scene-like structures
     *
     * Allows registration of individual draw calls for specific pipelines.
     *
     * The concept of 'drawable objects' is not inherent in the scene, but
     * can instead be implemented, for example as a collection of draw
     * functions with associated external state. This keeps the fundamental
     * render state incredibly flexible, as it does not make any assumptions
     * about the components and processing stages of complete renderable
     * entities.
     */
    class RasterSceneBase
    {
    public:
        RasterSceneBase(const RasterSceneBase&) = delete;
        RasterSceneBase(RasterSceneBase&&) noexcept = delete;
        RasterSceneBase& operator=(const RasterSceneBase&) = delete;
        RasterSceneBase& operator=(RasterSceneBase&&) noexcept = delete;

        RasterSceneBase() = default;
        ~RasterSceneBase() noexcept = default;

    private:
        /**
         * Registers one drawable with one command buffer recording function
         * at a specific pipeline.
         */
        struct DrawableExecutionRegistration
        {
            struct RegistrationIndex
            {
                RegistrationIndex(RenderStage::ID stage,
                                  SubPass::ID sub,
                                  Pipeline::ID pipeline,
                                  ui32 i);

                RenderStage::ID renderStage;
                SubPass::ID subPass;
                Pipeline::ID pipeline;
                ui32 indexInRegistrationArray;
            };

            struct ID
            {
                ID() = default;
                explicit ID(DrawableExecutionRegistration& r)
                    : regIndex(r.indexInRegistrationArray.get()) {}

                RegistrationIndex* regIndex{ nullptr };
            };

            DrawableExecutionRegistration() = default;

            /**
             * @brief Construct a registration
             *
             * Leave the index empty, fill it in SceneBase::insertRegistration().
             */
            DrawableExecutionRegistration(
                std::unique_ptr<RegistrationIndex> indexStruct,
                DrawableFunction func);

            // Allows me to modify pointer of all ID structs remotely
            std::unique_ptr<RegistrationIndex> indexInRegistrationArray;

            // Entry data
            DrawableFunction recordFunction;
        };

        /**
         * @brief A unique wrapper for drawable registrations at a scene
         *
         * Automatically unregisters a referenced drawable registration ID from
         * the scene.
         */
        struct UniqueDrawableRegistrationId
        {
        public:
            UniqueDrawableRegistrationId(const UniqueDrawableRegistrationId&) = delete;
            auto operator=(const UniqueDrawableRegistrationId&) -> UniqueDrawableRegistrationId& = delete;

            UniqueDrawableRegistrationId() = default;
            UniqueDrawableRegistrationId(UniqueDrawableRegistrationId&&) noexcept;
            UniqueDrawableRegistrationId(DrawableExecutionRegistration::ID id,
                                          RasterSceneBase& scene);
            ~UniqueDrawableRegistrationId();

            auto operator=(UniqueDrawableRegistrationId&&) noexcept -> UniqueDrawableRegistrationId&;

            auto getScene() -> RasterSceneBase*;

        private:
            DrawableExecutionRegistration::ID id;
            RasterSceneBase* scene{ nullptr };
        };

        /**
         * @brief Transient wrapper that can construct a unique ID on demand
         *
         * Purely used as an r-value. Can be converted to either a plain
         * registration ID or a unique registration wrapper.
         */
        class MaybeUniqueRegistrationId
        {
        public:
            MaybeUniqueRegistrationId(DrawableExecutionRegistration::ID id, RasterSceneBase& scene)
                : id(id), scene(&scene)
            {}

            inline operator DrawableExecutionRegistration::ID() && {
                return id;
            }

            inline operator UniqueDrawableRegistrationId() && {
                return { id, *scene };
            }

            inline auto makeUnique() && -> UniqueDrawableRegistrationId {
                return { id, *scene };
            }

        private:
            DrawableExecutionRegistration::ID id;
            RasterSceneBase* scene;
        };

    public:
        using RegistrationID = DrawableExecutionRegistration::ID;
        using UniqueRegistrationID = UniqueDrawableRegistrationId;

        /**
         * A proxy to a vector of pipelines which also holds a lock.
         */
        class PipelineListProxy
        {
        public:
            using const_iterator = std::vector<Pipeline::ID>::const_iterator;

            PipelineListProxy() = delete;
            PipelineListProxy(const PipelineListProxy&) = delete;
            PipelineListProxy& operator=(const PipelineListProxy&) = delete;
            PipelineListProxy& operator=(PipelineListProxy&&) noexcept = delete;

            PipelineListProxy(PipelineListProxy&&) noexcept = default;
            ~PipelineListProxy() = default;

            auto begin() const -> const_iterator;
            auto end() const -> const_iterator;

            bool empty() const;
            auto size() const -> size_t;

        private:
            friend class RasterSceneBase;
            PipelineListProxy(std::shared_lock<std::shared_mutex> lock,
                          const std::vector<Pipeline::ID>& p)
                : lock(std::move(lock)), pipelines(p)
            {}

            std::shared_lock<std::shared_mutex> lock;
            const std::vector<Pipeline::ID>& pipelines;
        };

        /**
         * @brief Register a draw function at the scene.
         *
         * The draw function implements a draw call. It is always specific
         * to a render stage, a subpass within that stage, and a pipeline
         * in which the draw call takes place.
         *
         * Pass the returned ID to `SceneBase::unregisterDrawFunction` to
         * remove the draw call from the scene, or use the unique handle
         * which takes care of this automatically.
         *
         * @return MaybeUniqueRegistrationId An ID which can be used to
         *         reference this specific draw call. Can be converted to
         *         either a plain ID or a handle with self-managed lifetime
         *         (unique_ptr semantics).
         *         The plain (non-unique) ID consumes less memory.
         */
        auto registerDrawFunction(
            RenderStage::ID stage,
            SubPass::ID subpass,
            Pipeline::ID usedPipeline,
            DrawableFunction commandBufferRecordingFunction
        ) -> MaybeUniqueRegistrationId;

        /**
         * @brief Remove a draw function from the scene.
         *
         * The UniqueRegistrationID handle calls this automatically on
         * destruction.
         */
        void unregisterDrawFunction(RegistrationID id);

        /**
         * @brief Get all pipelines used in a subpass
         *
         * Locks the returned list of pipelines with a shared read-only
         * lock. Adding new pipelines to the scene in the same thread that
         * holds the returned `PipelineListProxy` object *will* result in a
         * deadlock!
         *
         * This function is invoked internally by the renderer.
         */
        auto iterPipelines(RenderStage::ID stage, SubPass::ID subPass) const noexcept
            -> PipelineListProxy;

        /**
         * @brief Invoke all registered draw functions of a subpass and pipeline
         *
         * This function is invoked internally by the renderer.
         */
        void invokeDrawFunctions(
            RenderStage::ID stage,
            RenderPass& renderPass,
            SubPass::ID subPass,
            Pipeline::ID pipelineId,
            Pipeline& pipeline,
            vk::CommandBuffer cmdBuf
        ) const;

    private:
        template<typename T>
        class LockedStorage
        {
        public:
            LockedStorage(const LockedStorage&) = delete;
            LockedStorage& operator=(const LockedStorage&) = delete;

            LockedStorage() = default;
            LockedStorage(LockedStorage&&) noexcept = default;
            ~LockedStorage() noexcept = default;

            LockedStorage& operator=(LockedStorage&&) noexcept = default;

            auto read() const -> std::pair<const T&, std::shared_lock<std::shared_mutex>>;
            auto write()      -> std::pair<T&, std::unique_lock<std::shared_mutex>>;

        private:
            mutable u_ptr<std::shared_mutex> mutex{ new std::shared_mutex };

            // Must be a unique_ptr because a pipeline list could be moved
            // while being accessed
            u_ptr<T> data{ new T };
        };

        template<typename T> using PerRenderStage = data::IndexMap<RenderStage::ID::IndexType, T>;
        template<typename T> using PerSubpass = data::IndexMap<SubPass::ID::IndexType, T>;
        template<typename T> using PerPipeline = data::IndexMap<Pipeline::ID::IndexType, T>;

        auto readDrawCalls(RenderStage::ID renderStage,
                           SubPass::ID subPass,
                           Pipeline::ID pipelineId) const
            -> std::pair<
                const std::vector<DrawableExecutionRegistration>&,
                std::shared_lock<std::shared_mutex>
            >;

        auto writeDrawCalls(RenderStage::ID renderStage,
                            SubPass::ID subPass,
                            Pipeline::ID pipelineId)
            -> std::pair<
                std::vector<DrawableExecutionRegistration>&,
                std::unique_lock<std::shared_mutex>
            >;

        mutable std::shared_mutex drawRegsMutex;
        PerRenderStage<
            PerSubpass<
                PerPipeline<
                    LockedStorage<std::vector<DrawableExecutionRegistration>>
                >
            >
        > drawRegistrations;

        // Auxiliaries for pipeline management
        void tryInsertPipeline(RenderStage::ID renderStageType,
                               SubPass::ID subpass,
                               Pipeline::ID pipeline);
        void removePipeline(RenderStage::ID renderStageType,
                            SubPass::ID subpass,
                            Pipeline::ID pipeline);

        mutable std::shared_mutex uniquePipelinesMutex;
        mutable std::shared_mutex uniquePipelinesVectorMutex;
        PerRenderStage<PerSubpass<std::unordered_set<Pipeline::ID>>> uniquePipelines;
        PerRenderStage<PerSubpass<std::vector<Pipeline::ID>>> uniquePipelinesVector;
    };
} // namespace trc
