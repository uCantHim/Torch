cmake_minimum_required(VERSION 3.14)

project(torch-imgui)

add_library(torch-imgui)
torch_default_compile_options(torch-imgui)

target_sources(torch-imgui PRIVATE ImguiIntegration.cpp)
target_include_directories(torch-imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(torch-imgui PUBLIC torch imgui)
