target_sources(
    torch
    PUBLIC
        Drawable.h
        CommandCollector.h
        Pipeline.h
        Renderpass.h
        BasicScene.h
        Shader.h
        StaticDrawable.h
        StaticDrawable.inl
    PRIVATE
        Drawable.cpp
        Renderpass.cpp
        BasicScene.cpp
)

add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(NewArchTest main.cpp)

target_link_libraries(NewArchTest PUBLIC vk_base torch)
target_include_directories(NewArchTest PUBLIC ../vk_base/include)
