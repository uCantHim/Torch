#pragma once

#include <concepts>

#include "RenderStage.h"
#include "RenderPass.h"
#include "RenderLayout.h"
#include "PipelineRegistry.h"

namespace trc
{
    struct DrawConfig;

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig
    {
    public:
        explicit RenderConfig(RenderLayout layout);
        virtual ~RenderConfig() = default;

        virtual void preDraw(const DrawConfig& draw) = 0;
        virtual void postDraw(const DrawConfig& draw) = 0;

        virtual auto getPipeline(Pipeline::ID id) -> Pipeline& = 0;

        auto getLayout() -> RenderLayout&;

    protected:
        RenderLayout layout;
    };

    /**
     * @brief An implementation helper for custom RenderConfigs
     */
    template<typename Derived>
    class RenderConfigCrtpBase : public RenderConfig
    {
    public:
        /**
         * @brief
         */
        inline RenderConfigCrtpBase(const Instance& instance, RenderLayout layout);

        inline auto getPipeline(Pipeline::ID id) -> Pipeline& override;
        inline auto getPipelineStorage() -> PipelineStorage<Derived>&;

    private:
        u_ptr<PipelineStorage<Derived>> pipelineStorage;
    };

    /**
     * @brief A type that implements RenderConfig
     */
    template<typename T>
    concept RenderConfigType = std::derived_from<T, RenderConfig>;



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
