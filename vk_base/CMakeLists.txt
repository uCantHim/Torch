cmake_minimum_required(VERSION 3.13)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

project(VulkanBase)

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#############
# The library

add_library(vk_base)
add_subdirectory(src)

###################
# External packages

find_package(PkgConfig REQUIRED) # For GLFW

find_package(Vulkan REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(glm REQUIRED)
find_package(DevIL REQUIRED)

target_link_libraries(
    vk_base
    PUBLIC
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
        ${CMAKE_CURRENT_LIST_DIR}/include/vkb
)

if (LINUX)
    target_link_libraries(vk_base PUBLIC pthread)
endif()


#################
# Compile options

target_compile_options(vk_base PUBLIC -fPIC)
target_compile_features(vk_base PUBLIC cxx_std_20)

if (NOT MSVC)
    target_compile_options(vk_base PRIVATE -Wall -Wextra -Wpedantic)
endif()
