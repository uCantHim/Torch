


template<typename Derived, SubPass::ID SubPass, GraphicsPipeline::ID Pipeline>
StaticPipelineRenderInterface<Derived, SubPass, Pipeline>::StaticPipelineRenderInterface()
{
    // Assert that Derived is actually a subclass of this template instantiation
    static_assert(std::is_base_of_v<Self, Derived>,
                  "Template parameter Derived must be a type derived from"
                  " StaticPipelineRenderInterface.");
    // Assert that Derived also inherits from SceneRegisterable
    static_assert(std::is_base_of_v<SceneRegisterable, Derived>,
                  "Classes derived from StaticPipelineRenderInterface must also inherit"
                  " SceneRegisterable.");

    static_cast<Derived*>(this)->usePipeline(
        SubPass,
        Pipeline,
        std::bind(
            static_cast<RecordFuncType>(&Derived::recordCommandBuffer),
            static_cast<Derived*>(this),
            PipelineIndex<Pipeline>(),
            std::placeholders::_1
        )
    );
}
