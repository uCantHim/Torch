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
        /**
         * @brief
         */
        inline RenderConfigCrtpBase(const Instance& instance, RenderLayout layout);

        inline auto getPipeline(Pipeline::ID id) -> Pipeline& final;
        inline auto getPipelineStorage() -> PipelineStorage<Derived>&;

    private:
        u_ptr<PipelineStorage<Derived>> pipelineStorage;
    };



    // ------------------------- //
    //      Implementations      //
    // ------------------------- //

    template<typename Derived>
    inline RenderConfigCrtpBase<Derived>::RenderConfigCrtpBase(
        const Instance& instance,
        RenderLayout layout)
        :
        RenderConfig(std::move(layout)),
        pipelineStorage(
            PipelineRegistry<Derived>::createStorage(instance, static_cast<Derived&>(*this))
        )
    {}

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
