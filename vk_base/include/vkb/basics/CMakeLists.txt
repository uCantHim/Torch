target_sources(
    vk_base
    PUBLIC
        Device.h
        Instance.h
        PhysicalDevice.h
        Swapchain.h
        VulkanDebug.h
)

target_include_directories(vk_base PRIVATE ${CMAKE_CURRENT_LIST_DIR})
