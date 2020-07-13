add_subdirectory(basics)

target_sources(
    vk_base
    PUBLIC
        Buffer.h
        Memory.h
        FrameSpecificObject.h
        Image.h
        Logger.h
        MemoryPool.h
        PoolProvider.h
        QueueProvider.h
        ShaderProgram.h
        VulkanEXT.h
        VulkanBase.h
)

target_include_directories(vk_base PRIVATE ${CMAKE_CURRENT_LIST_DIR})
