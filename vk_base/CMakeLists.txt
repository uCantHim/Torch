cmake_minimum_required(VERSION 3.13)

project(VulkanBase)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#############
# Third-party

find_package(Threads)
find_package(PkgConfig REQUIRED) # For GLFW

find_package(Vulkan REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(glm REQUIRED)
find_package(DevIL REQUIRED)

#############
# The library

add_library(vk_base)

torch_default_compile_options(vk_base)
target_compile_definitions(vk_base PUBLIC
    $<$<BOOL:${TORCH_DEBUG}>:TRC_DEBUG>
)

add_subdirectory(src)

target_link_libraries(
    vk_base
    PUBLIC
        Threads::Threads
        Vulkan::Vulkan
        ${GLFW_LIBRARIES}
        ${IL_LIBRARIES}
)

target_include_directories(
    vk_base
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${IL_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/vkb
)
