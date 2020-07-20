add_subdirectory(basics)

target_sources(
    vk_base
    PRIVATE
        Buffer.cpp
        Memory.cpp
        Image.cpp
        Logger.cpp
        MemoryPool.cpp
        QueueProvider.cpp
        ShaderProgram.cpp
        VulkanBase.cpp
)
