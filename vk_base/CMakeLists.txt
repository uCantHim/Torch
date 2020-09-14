cmake_minimum_required(VERSION 3.13)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

project(VulkanBase)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#############
# The library

add_library(vk_base)
add_subdirectory(src)
add_subdirectory(include/vkb)

###################
# External packages

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(DevIL REQUIRED)

target_link_libraries(
    vk_base
    PUBLIC
        Vulkan::Vulkan
        glfw
        ${IL_LIBRARIES}
)

target_include_directories(
    vk_base
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${IL_INCLUDE_DIR}
)

if (LINUX)
    target_link_libraries(vk_base PUBLIC pthread)
endif()


#################
# Compile options

target_compile_options(vk_base PUBLIC -fPIC)

if (NOT MSVC)
    target_compile_options(vk_base PRIVATE -Wall -Wextra -Wpedantic)
endif()
