add_subdirectory(basics)
add_subdirectory(event)

target_sources(
    vk_base
    PRIVATE
        Buffer.cpp
        Memory.cpp
        Image.cpp
        Logger.cpp
        MemoryPool.cpp
        ShaderProgram.cpp
        VulkanBase.cpp
)
