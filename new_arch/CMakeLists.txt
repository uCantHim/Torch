target_sources(
    torch
    PUBLIC
        CommandCollector.h
        DrawableBase.h
        Node.h
        Pipeline.h
        PipelineBuilder.h
        Renderpass.h
        Shader.h
    PRIVATE
        DrawableBase.cpp
        Node.cpp
        PipelineBuilder.cpp
        Renderpass.cpp
)

add_subdirectory(base)
add_subdirectory(utils)
add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(NewArchTest main.cpp)

target_link_libraries(NewArchTest PUBLIC vk_base torch)
target_include_directories(NewArchTest PUBLIC ../vk_base/include)
