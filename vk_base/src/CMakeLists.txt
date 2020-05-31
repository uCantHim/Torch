add_subdirectory(basics)

target_sources(
    vk_base
    PRIVATE
        Buffer.cpp
        Image.cpp
        Logger.cpp
        PoolProvider.cpp
        QueueProvider.cpp
        ShaderProgram.cpp
        VulkanBase.cpp
)
