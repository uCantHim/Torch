add_subdirectory(basics)
add_subdirectory(event)
add_subdirectory(util)

target_sources(
    vk_base
    PRIVATE
        Buffer.cpp
        ExclusiveQueue.cpp
        Memory.cpp
        Image.cpp
        ImageUtils.cpp
        Logger.cpp
        MemoryPool.cpp
        QueueManager.cpp
        ShaderProgram.cpp
        VulkanBase.cpp
)
