#pragma once

#include <concepts>

#include "RenderStage.h"
#include "RenderPass.h"
#include "PipelineRegistry.h"
#include "RenderGraph.h"

namespace trc
{
    struct DrawConfig;

    /**
     * @brief A configuration of an entire render cycle
     */
    class RenderConfig
    {
    public:
        RenderConfig() = default;
        virtual ~RenderConfig() = default;

        virtual void preDraw(const DrawConfig& draw) = 0;
        virtual void postDraw(const DrawConfig& draw) = 0;

        virtual auto getPipeline(Pipeline::ID id) -> Pipeline& = 0;

        auto getGraph() -> RenderGraph&;
        auto getGraph() const -> const RenderGraph&;

    protected:
        RenderGraph graph;
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
        inline RenderConfigCrtpBase(const Instance& instance);

        inline auto getPipeline(Pipeline::ID id) -> Pipeline& override;

    private:
        u_ptr<PipelineStorage<Derived>> pipelineStorage;

        // TODO: Asset registry here
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
    inline RenderConfigCrtpBase<Derived>::RenderConfigCrtpBase(const Instance& instance)
        :
        pipelineStorage(
            PipelineRegistry<Derived>::createStorage(instance, static_cast<Derived&>(*this))
        )
    {}

    template<typename Derived>
    inline auto RenderConfigCrtpBase<Derived>::getPipeline(Pipeline::ID id) -> Pipeline&
    {
        return pipelineStorage->get(id);
    }
} // namespace trc
