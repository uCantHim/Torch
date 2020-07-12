#pragma once

#include "vkb/VulkanBase.h"

class VulkanDrawableInterface;

namespace internal
{
    /**
     * @brief Records used pipelines for a class
     *
     * Every instantiation stores pipelines used by a specific class.
     * This is used by the engine when recording commands.
     *
     * Don't inherit from this directly!
     * Add a pipeline by inheriting Derived from has_pipeline.
     */
    template<class Derived, class PipelineType>
    class pipeline_helper
    {
    protected:
        /**
         * @brief Assigns a pipeline to a subpass index
         *
         * If this is called multiple times, the pipeline for the subpass
         * will be overridden.
         *
         * @param uint32_t subpassIndex  Index of the subpass that executes
         *                               the pipeline
         * @param PipelineType& pipeline The pipeline that is executed in the
         *                               specified subpass
         */
        static inline void _add_pipeline(uint32_t subpassIndex, PipelineType& pipeline)
        {
            if (pipelinesPerSubpass.size() <= subpassIndex)
                pipelinesPerSubpass.resize(subpassIndex + 1, nullptr);

            pipelinesPerSubpass[subpassIndex] = &pipeline;
        }

        /**
         * @return The pipeline for a specific subpass
         *
         * Performs extra validity checks in debug mode, may throw:
         *
         *  - std::invalid_argument if the subpass index is greater than the
         *    largest subpass declared by has_pipeline
         *
         *  - std::runtime_error in the subpass index is specified but the stored
         *    pipeline is nullptr. This is most probably a bug.
         */
        static inline constexpr auto _get_pipeline(uint32_t subpassIndex) -> PipelineType&
        {
            if constexpr (vkb::debugMode)
            {
                if (subpassIndex >= pipelinesPerSubpass.size()) {
                    throw std::invalid_argument("Class does not define a pipeline for subpass "
                                                + std::to_string(subpassIndex) + ".");
                }
                if (pipelinesPerSubpass[subpassIndex] == nullptr)
                {
                    throw std::runtime_error(
                        "Pipeline at subpass " + std::to_string(subpassIndex) + " exists but "
                        "was nullptr. This should not happen. Maybe the user specified a gap "
                        "in subsequent has_pipelines? Otherwise this is likely a bug.");
                }
            }
            return *pipelinesPerSubpass[subpassIndex];
        }

    private:
        static inline std::vector<PipelineType*> pipelinesPerSubpass;
        static inline std::vector<size_t> pipelineIndices;

    };
} // namespace internal


/**
 * @brief Inherit from this to assign a Pipeline to a Drawable
 *
 * Drawables must have a pipeline assigned this way for every subpass
 * they are used in.
 *
 * @tparam Derived       The derived type, CRTP style
 * @tparam PipelineType  Type of the used pipeline (graphics, ...)
 * @tparam PipelineIndex Storage index of the used pipeline
 * @tparam SubpassIndex  The subpass that the pipeline is used in
 */
template<class Derived, class PipelineType, uint32_t PipelineIndex, uint32_t SubpassIndex>
class has_pipeline : private internal::pipeline_helper<Derived, PipelineType>
{
    friend PipelineType;

    /**
     * Encapsulate the static assertion in a function because the Derived
     * type is incomplete at class template instantiation time. The
     * function is instantiated lazily (after the has_pipeline instance)
     * when it is actually called in the constructor. The Derived type is
     * complete at that time.
     */
    inline constexpr void _do_static_assertion() {
        static_assert(std::is_base_of_v<VulkanDrawableInterface, Derived>,
                      "A class derived from any instantiation of has_pipeline must "
                      "implement VulkanDrawableInterface! Recommendation: inherit "
                      "from VulkanDrawable.");
    }

    /**
     * Has to use a new pipeline when it's recreated, the old pipeline
     * in pipeline_helper would point to invalid memory.
     */
    class recreate_helper : public vkb::SwapchainDependentResource
    {
    public:
        void signalRecreateRequired() override {}
        void recreate(vkb::Swapchain&) override {}
        void signalRecreateFinished() override {
            auto pipeline = PipelineType::find(PipelineIndex);
            if (pipeline.has_value())
                internal::pipeline_helper<Derived, PipelineType>::_add_pipeline(SubpassIndex, *pipeline.value());
        }
    };
    static inline recreate_helper _recreate_helper;

protected:
    inline constexpr has_pipeline()
    {
        _do_static_assertion();

        // Adds the pipeline to the pipeline helper and ensures that the static
        // recreate_helper member is actually created. Idk why but gcc doesn't
        // create that thing if I don't use one of its functions. Which I find
        // quite audacious.
        _recreate_helper.signalRecreateFinished();
    }
};
