target_sources(
    torch
    PUBLIC
        CommandCollector.h
        DrawableBase.h
        Pipeline.h
        Renderpass.h
        SceneBase.h
        SceneRegisterable.h
        Shader.h
        DrawableStatic.h
        DrawableStatic.inl
    PRIVATE
        DrawableBase.cpp
        Renderpass.cpp
        SceneBase.cpp
        SceneRegisterable.cpp
)

add_subdirectory(utils)
add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(NewArchTest main.cpp)

target_link_libraries(NewArchTest PUBLIC vk_base torch)
target_include_directories(NewArchTest PUBLIC ../vk_base/include)
