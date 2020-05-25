cmake_minimum_required(VERSION 3.13)

project(VulkanAppImproved)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(Main main.cpp)

target_link_libraries(Main PUBLIC torch)

target_include_directories(
    Main
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../src/"
        "${CMAKE_CURRENT_LIST_DIR}/../vk_base/src/"
)

target_sources(
    Main
    PUBLIC
        Quad.h
        Settings.h
        TriangleList.h
        VulkanApp.h
    PRIVATE
        Quad.cpp
        TriangleList.cpp
        VulkanApp.cpp
)
