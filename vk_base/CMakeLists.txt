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
find_package(PNG REQUIRED)

# Load CImg
FetchContent_Declare(
    cimg
    GIT_REPOSITORY https://github.com/dtschump/CImg.git
    GIT_TAG        927fee511fe4fcc1ae5cdf2365a60048fe4ff935
)
FetchContent_MakeAvailable(cimg)


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
        torch_util
    PRIVATE
        PNG::PNG
)

target_include_directories(
    vk_base
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/vkb
        ${cimg_SOURCE_DIR}
)
