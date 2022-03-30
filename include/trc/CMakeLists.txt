target_precompile_headers(torch
    INTERFACE
    Types.h
    VulkanInclude.h

    core/Instance.h
    core/PipelineRegistry.h
    core/SceneBase.h

    PRIVATE
    Types.h
    VulkanInclude.h
)
