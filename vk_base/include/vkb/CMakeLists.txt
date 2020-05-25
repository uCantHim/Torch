add_subdirectory(basics)

target_sources(
    vk_base
    PUBLIC
        Buffer.h
        FrameSpecificObject.h
        Logger.h
        PoolProvider.h
        QueueProvider.h
        ShaderProgram.h
        VulkanEXT.h
        VulkanBase.h
)

target_include_directories(vk_base PRIVATE ${CMAKE_CURRENT_LIST_DIR})
