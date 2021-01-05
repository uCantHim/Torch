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
        StaticInit.cpp
        VulkanBase.cpp
)
