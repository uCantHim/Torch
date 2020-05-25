add_subdirectory(basics)

target_sources(
    vk_base
    PRIVATE
        Buffer.cpp
        Logger.cpp
        PoolProvider.cpp
        QueueProvider.cpp
        ShaderProgram.cpp
        VulkanBase.cpp
)

# target_include_directories(vk_base PRIVATE ${CMAKE_CURRENT_LIST_DIR})
