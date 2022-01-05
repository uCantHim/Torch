#pragma once

#include "RenderConfiguration.h"
#include "PipelineRegistry.h"

namespace trc
{
    /**
     * @brief An implementation helper for custom RenderConfigs
     *
     * Contains a pipeline storage for the derived type.
     */
    template<typename Derived>
    class RenderConfigCrtpBase : public RenderConfig
    {
    public:
        RenderConfigCrtpBase(const Instance& instance, RenderLayout layout);
        ~RenderConfigCrtpBase();

        inline auto getPipeline(Pipeline::ID id) -> Pipeline& final;
        inline auto getPipelineStorage() -> PipelineStorage<Derived>&;

    private:
        static inline std::atomic<ui32> instanceCount{ 0 };
        static inline u_ptr<PipelineStorage<Derived>> pipelineStorage{ nullptr };
    };



    // ------------------------- //
    //      Implementations      //
    // ------------------------- //

    template<typename Derived>
    inline RenderConfigCrtpBase<Derived>::RenderConfigCrtpBase(
        const Instance& instance,
        RenderLayout layout)
        :
        RenderConfig(std::move(layout))
    {
        if (pipelineStorage == nullptr)
        {
            pipelineStorage = PipelineRegistry<Derived>::createStorage(
                instance,
                static_cast<Derived&>(*this)
            );
        }
        ++instanceCount;
    }

    template<typename Derived>
    inline RenderConfigCrtpBase<Derived>::~RenderConfigCrtpBase()
    {
        /**
         * The pipeline storage is static so that we have only one pipeline
         * instance for all render configs of the same type, but we still
         * have to destroy the pipeline storage before the device is
         * destroyed.
         */
        --instanceCount;
        if (instanceCount == 0) {
            pipelineStorage.reset();
        }
    }

    template<typename Derived>
    inline auto RenderConfigCrtpBase<Derived>::getPipeline(Pipeline::ID id) -> Pipeline&
    {
        return pipelineStorage->get(id);
    }

    template<typename Derived>
    inline auto RenderConfigCrtpBase<Derived>::getPipelineStorage() -> PipelineStorage<Derived>&
    {
        return *pipelineStorage;
    }
} // namespace trc
