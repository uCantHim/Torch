add_subdirectory(basics)
add_subdirectory(util)
add_subdirectory(event)

target_sources(
    vk_base
    PUBLIC
        Buffer.h
        Memory.h
        FrameSpecificObject.h
        Image.h
        Logger.h
        MemoryPool.h
        QueueProvider.h
        ShaderProgram.h
        VulkanEXT.h
        VulkanBase.h
)

target_include_directories(vk_base PRIVATE ${CMAKE_CURRENT_LIST_DIR})
