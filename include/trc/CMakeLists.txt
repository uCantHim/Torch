target_precompile_headers(torch
    INTERFACE
    Types.h
    VulkanInclude.h

    # core
    Instance.h
    PipelineRegistry.h
    SceneBase.h

    PRIVATE
    Types.h
    VulkanInclude.h
)
