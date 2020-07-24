cmake_minimum_required(VERSION 3.13)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

project(Torch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

add_library(torch)
add_subdirectory(vk_base)
add_subdirectory(src)
add_subdirectory(include/trc)
add_subdirectory(shaders)

# Does not work with the new version yet
# add_subdirectory(example)

target_link_libraries(torch PUBLIC vk_base fbxsdk)

target_include_directories(torch PUBLIC include)
target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR}/vk_base/include)


#################
# Compile options

target_compile_options(torch PUBLIC -fPIC)

if (NOT MSVC)
    target_compile_options(torch PRIVATE -Wall -Wextra -Wpedantic)
else()
    target_compile_options(torch PRIVATE /W4)
endif()
